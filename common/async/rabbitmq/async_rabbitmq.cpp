/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_RABBITMQ

#include <cassert>
#include <time.h>
#include "common/async/rabbitmq/data.h"
#include <rabbitmq-c/tcp_socket.h>

namespace async {
namespace rabbitmq {

// io线程的函数
std::function<void(std::function<void()>)> gIoThr = nullptr;
// 超时时间
uint32_t gTimeout = 5;
// 最大channel数
uint32_t gMaxChannel = 1; // 目前不允许线程共享conn

////////////////////////////////////////////////////////////////////////////////////

bool checkError(amqp_rpc_reply_t x, char const *context) {
    if (AMQP_RESPONSE_NORMAL == x.reply_type) {
        return true;
    }

    if (AMQP_RESPONSE_NONE == x.reply_type) {
        rabbitLog("[error] failed to call %s: missing RPC reply type", context);
    }
    else if (AMQP_RESPONSE_LIBRARY_EXCEPTION == x.reply_type) {
        rabbitLog("[error] failed to call %s: %s", context, amqp_error_string2(x.library_error));
    }
    else if (AMQP_RESPONSE_SERVER_EXCEPTION == x.reply_type) {
        if (AMQP_CONNECTION_CLOSE_METHOD == x.reply.id) {
            amqp_connection_close_t *m = (amqp_connection_close_t *)x.reply.decoded;
            rabbitLog("[error] failed to call %s: server connection error %uh, message: %.*s", 
                context,
                m->reply_code,
                (int)m->reply_text.len,
                (char *)m->reply_text.bytes
            );
        }
        else if (AMQP_CHANNEL_CLOSE_METHOD == x.reply.id) {
            amqp_channel_close_t *m = (amqp_channel_close_t *)x.reply.decoded;
            rabbitLog("[error] failed to call %s: server channel error %uh, message: %.*s", 
                  context, 
                  m->reply_code, 
                  (int)m->reply_text.len,
                  (char *)m->reply_text.bytes
            );
        }
        else {
            rabbitLog("[error] failed to call %s: unknown server error, method id 0x%08X", context, x.reply.id);
        }
    }

    return false;
}

bool checkConn(amqp_rpc_reply_t x) {
    if (AMQP_RESPONSE_LIBRARY_EXCEPTION == x.reply_type) {
        return false;
    }
    else if (AMQP_RESPONSE_SERVER_EXCEPTION == x.reply_type) {
        if (AMQP_CONNECTION_CLOSE_METHOD == x.reply.id) {
            return false;
        }
    }

    return true;
}

bool checkChannel(amqp_rpc_reply_t x) {
    if (AMQP_RESPONSE_SERVER_EXCEPTION == x.reply_type) {
        if (AMQP_CHANNEL_CLOSE_METHOD == x.reply.id) {
            return false;
        }
    }

    return true;
}

void threadExchangeCmd(RabbitChannelPtr ch, std::shared_ptr<DeclareExchangeCmd> cmd, RabbitRspDataPtr rsp) {
    //rabbitLog("enter declare exchange%s", "");
    amqp_exchange_declare(ch->conn, 
        ch->chId, 
        amqp_cstring_bytes(cmd->name.c_str()),
        amqp_cstring_bytes(cmd->type.c_str()), 
        cmd->passive ? 1 : 0, /*passive*/
        cmd->durable ? 1 : 0, /*durable*/
        cmd->autoDelete ? 1 : 0, /*auto_delete*/
        0,  /*internal*/
        amqp_empty_table
    );

    rsp->reply = amqp_get_rpc_reply(ch->conn);
    if (checkError(rsp->reply, "amqp_exchange_declare")) {
        rsp->ok = true;
    } else {
        rsp->ok = false;
    }

    //rabbitLog("leave declare exchange%s", "");
}

void threadQueueCmd(RabbitChannelPtr ch, std::shared_ptr<DeclareQueueCmd> cmd, RabbitRspDataPtr rsp) {
    //rabbitLog("enter declare queue%s", "");
    amqp_queue_declare(ch->conn,
        ch->chId,
        amqp_cstring_bytes(cmd->name.c_str()),
        cmd->passive ? 1 : 0, /*passive*/
        cmd->durable ? 1 : 0, /*durable*/
        cmd->exclusive ? 1 : 0, /*durable*/
        cmd->autoDelete ? 1 : 0, /*auto_delete*/
        amqp_empty_table
    );
    
    rsp->reply = amqp_get_rpc_reply(ch->conn);
    if (checkError(rsp->reply, "amqp_queue_declare")) {
        rsp->ok = true;
    } else {
        rsp->ok = false;
    }

    //rabbitLog("leave declare queue%s", "");
}

void threadBindCmd(RabbitChannelPtr ch, std::shared_ptr<BindCmd> cmd, RabbitRspDataPtr rsp) {
    amqp_queue_bind(ch->conn, 
        ch->chId, 
        amqp_cstring_bytes(cmd->queue.c_str()),
        amqp_cstring_bytes(cmd->exchange.c_str()), 
        amqp_cstring_bytes(cmd->routingKey.c_str()),
        amqp_empty_table
    );

    rsp->reply = amqp_get_rpc_reply(ch->conn);
    if (checkError(rsp->reply, "amqp_queue_bind")) {
        rsp->ok = true;
    } else {
        rsp->ok = false;
    }
}

void threadUnbindCmd(RabbitChannelPtr ch, std::shared_ptr<UnbindCmd> cmd, RabbitRspDataPtr rsp) {
    amqp_queue_unbind(ch->conn, 
        ch->chId, 
        amqp_cstring_bytes(cmd->queue.c_str()),
        amqp_cstring_bytes(cmd->exchange.c_str()),
        amqp_cstring_bytes(cmd->routingKey.c_str()), 
        amqp_empty_table
    );

    rsp->reply = amqp_get_rpc_reply(ch->conn);
    if (checkError(rsp->reply, "amqp_queue_bind")) {
        rsp->ok = true;
    } else {
        rsp->ok = false;
    }
}

void threadPublishCmd(RabbitChannelPtr ch, std::shared_ptr<PublishCmd> cmd, RabbitRspDataPtr rsp) {
    amqp_basic_properties_t props;
    props._flags = 0;
    for (auto& item : cmd->properties) {
        if (item.first == "content-type") {
            props._flags |= AMQP_BASIC_CONTENT_TYPE_FLAG;
            props.content_type = amqp_cstring_bytes(item.second.c_str());
        } else if (item.first == "content-encoding") {
            props._flags |= AMQP_BASIC_CONTENT_ENCODING_FLAG;
            props.content_encoding = amqp_cstring_bytes(item.second.c_str());
        } else if (item.first == "message-id") {
            props._flags |= AMQP_BASIC_MESSAGE_ID_FLAG;
            props.message_id = amqp_cstring_bytes(item.second.c_str());
        } else if (item.first == "timestamp") {
            props._flags |= AMQP_BASIC_TIMESTAMP_FLAG;
            props.timestamp = std::stoull(item.second);
        } else if (item.first == "expiration") {
            props._flags |= AMQP_BASIC_EXPIRATION_FLAG;
            props.expiration = amqp_cstring_bytes(item.second.c_str());
        } else if (item.first == "delivery-mode") {
            props._flags |= AMQP_BASIC_DELIVERY_MODE_FLAG;
            props.delivery_mode = (uint8_t)atoi(item.second.c_str());
        } else if (item.first == "priority") {
            props._flags |= AMQP_BASIC_PRIORITY_FLAG;
            props.priority = (uint8_t)atoi(item.second.c_str());
        }
    }

    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type = amqp_cstring_bytes("text/plain");
    props.delivery_mode = 2; /* persistent delivery mode */

    amqp_basic_publish(ch->conn, 
        ch->chId,
        amqp_cstring_bytes(cmd->exchange.c_str()),
        amqp_cstring_bytes(cmd->routingKey.c_str()), 
        cmd->mandatory ? 1 : 0,
        cmd->immediate ? 1 : 0, 
        &props,
        amqp_cstring_bytes(cmd->msg.c_str())
    );

    rsp->reply = amqp_get_rpc_reply(ch->conn);
    if (checkError(rsp->reply, "amqp_basic_publish")) {
        rsp->ok = true;
    } else {
        rsp->ok = false;
    }
}

void threadDelQueueCmd(RabbitChannelPtr ch, std::shared_ptr<DeleteQueueCmd> cmd, RabbitRspDataPtr rsp) {
    amqp_queue_delete(ch->conn, 
        ch->chId, 
        amqp_cstring_bytes(cmd->name.c_str()), 
        cmd->if_unused ? 1 : 0, 
        cmd->if_empty ? 1 : 0
    );

    rsp->reply = amqp_get_rpc_reply(ch->conn);
    if (checkError(rsp->reply, "amqp_queue_delete")) {
        rsp->ok = true;
    } else {
        rsp->ok = false;
    }
}

void threadDelExchangeCmd(RabbitChannelPtr ch, std::shared_ptr<DeleteExchangeCmd> cmd, RabbitRspDataPtr rsp) {
    amqp_exchange_delete(ch->conn, 
        ch->chId,
        amqp_cstring_bytes(cmd->name.c_str()), 
        cmd->if_unused ? 1 : 0
    );

    rsp->reply = amqp_get_rpc_reply(ch->conn);
    if (checkError(rsp->reply, "amqp_queue_delete")) {
        rsp->ok = true;
    } else {
        rsp->ok = false;
    }
}

void threadGetCmd(RabbitChannelPtr ch, std::shared_ptr<GetCmd> cmd, RabbitRspDataPtr rsp) {
    rsp->reply = amqp_basic_get(ch->conn, 
        ch->chId, 
        amqp_cstring_bytes(cmd->queue.c_str()), 
        cmd->no_ack ? 1 : 0
    );

    if (checkError(rsp->reply, "amqp_basic_get")) {
        rsp->ok = true;
    } else {
        rsp->ok = false;
        return;
    }

    if (rsp->reply.reply.id == AMQP_BASIC_GET_EMPTY_METHOD) {
        return;
    }

    rsp->reply = amqp_read_message(ch->conn, ch->chId, &rsp->message, 0);
    if (checkError(rsp->reply, "amqp_read_message")) {
        rsp->ok = true;
    } else {
        rsp->ok = false;
        return;
    }
}

void threadAckCmd(RabbitChannelPtr ch, std::shared_ptr<AckCmd> cmd, RabbitRspDataPtr rsp) {
    int x = amqp_basic_ack(ch->conn, 
        ch->chId, 
        cmd->delivery_tag, 
        0
    );

    if (x < 0) {
        rsp->ok = false;
        rabbitLog("[error] failed to call amqp_basic_ack: %s", amqp_error_string2(x));
    } else {
        rsp->ok = true;
    }
}

void threadWatch(RabbitChannelPtr ch, std::shared_ptr<WatchCmd> cmd, RabbitRspDataPtr rsp) {
    amqp_basic_consume(ch->conn, 
        ch->chId, 
        amqp_cstring_bytes(cmd->queue.c_str()), 
        amqp_cstring_bytes(cmd->consumer_tag.c_str()), 
        cmd->no_local ? 1 : 0, 
        cmd->no_ack ? 1 : 0, 
        cmd->exclusive ? 1 : 0,
        amqp_empty_table
    );

    rsp->reply = amqp_get_rpc_reply(ch->conn);
    if (checkError(rsp->reply, "amqp_basic_consume")) {
        rsp->ok = true;
    } else {
        rsp->ok = false;
    }
}

////////////////////////////////////////////////////////////////////////////////////

RabbitThreadData* runningData() {
    thread_local static RabbitThreadData gThrData;
    return &gThrData;
}

GlobalData* globalData() {
    static GlobalData gData;
    return &gData;
}

RabbitCorePtr threadCreateCore(RabbitAddrPtr addr) {
    RabbitCorePtr core = std::make_shared<RabbitCore>();
    core->conn = amqp_new_connection();
    amqp_socket_t *socket = amqp_tcp_socket_new(core->conn);
    if (!socket) {
        rabbitLog("[error] failed to call amqp_tcp_socket_new%s", "");
        return nullptr;
    }

    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    int rc = amqp_socket_open_noblock(socket, addr->host.c_str(), addr->port, &tv);
    if (rc != AMQP_STATUS_OK) {
       rabbitLog("[error] failed to call amqp_socket_open_noblock: %s|%d", addr->host.c_str(), addr->port);
       return nullptr;
    }

    amqp_rpc_reply_t rpc_reply = amqp_login(core->conn, 
        addr->vhost.c_str(), 
        0, 
        AMQP_DEFAULT_FRAME_SIZE,
        AMQP_DEFAULT_HEARTBEAT, 
        AMQP_SASL_METHOD_PLAIN, 
        addr->user.c_str(), 
        addr->pwd.c_str()
    );

    if (!checkError(rpc_reply, "amqp_login")) {
        return nullptr;
    }

    amqp_set_rpc_timeout(core->conn, &tv);
    core->addr = addr;
    rabbitLog("connect rabbitmq: %s successfully", addr->id.c_str());
    return core;
}

RabbitChannelPtr threadCreateChannel(RabbitCorePtr core, uint32_t chId) {
    RabbitChannelPtr ch = std::make_shared<RabbitChannel>();
    ch->id = core->addr->id;
    ch->conn = core->conn;

    if (core->chId != 0) {
        ch->chId = core->chId;
    } else {
        ch->chId = chId == 0 ? 1 : chId;
        amqp_channel_open(ch->conn, ch->chId);
        if (checkError(amqp_get_rpc_reply(ch->conn), "amqp_channel_open")) {
            if (chId == 0) {
                core->chId = ch->chId;
            }
            rabbitLog("open channel: %s, %d successfully", ch->id.c_str(), ch->chId);
        }
        else {
            ch = nullptr;
        }
    }
   
    return ch;
}

void threadCloseChannel(RabbitCorePtr core, uint32_t chId) {
    rabbitLog("close channel: %s, %d", core->addr->id.c_str(), chId);
    amqp_channel_close_ok_t close_ok;
    amqp_send_method(core->conn, chId, AMQP_CHANNEL_CLOSE_OK_METHOD, &close_ok);
    amqp_maybe_release_buffers_on_channel(core->conn, chId);
    //checkError(amqp_channel_close(core->conn, chId, AMQP_CHANNEL_ERROR), "amqp_channel_close");
    core->chId = 0;
}

RabbitChannelPtr threadGetChannel(RabbitAddrPtr addr) {
    RabbitCorePtr core;

    auto gData = globalData();
    gData->lock.lock();

    auto& pool = gData->corePool[addr->id];
    if (!pool.valid.empty()) {
        core = pool.valid.front();
        pool.valid.pop_front();
    }

    gData->lock.unlock();

    if (!core) {
        core = threadCreateCore(addr);
        if (!core) {
            return nullptr;
        }
    }

    RabbitChannelPtr ch = threadCreateChannel(core, 0);
    if (ch) {
        ch->core = core;
    }

    return ch;
}

void threadProcess(RabbitReqDataPtr req, RabbitThreadData* tData) {
    RabbitRspDataPtr rsp = std::make_shared<RabbitRspData>();
    rsp->req = req;

    do {
        auto ch = threadGetChannel(req->addr);
        if (!ch) {
            break;
        }

        if (req->cmd->cmdType == declare_exchange_cmd) {
            threadExchangeCmd(ch, std::static_pointer_cast<DeclareExchangeCmd>(req->cmd), rsp);
        } else if (req->cmd->cmdType == declare_queue_cmd) {
            threadQueueCmd(ch, std::static_pointer_cast<DeclareQueueCmd>(req->cmd), rsp);
        } else if (req->cmd->cmdType == bind_cmd) {
            threadBindCmd(ch, std::static_pointer_cast<BindCmd>(req->cmd), rsp);
        } else if (req->cmd->cmdType == unbind_cmd) {
            threadUnbindCmd(ch, std::static_pointer_cast<UnbindCmd>(req->cmd), rsp);
        } else if (req->cmd->cmdType == publish_cmd) {
            threadPublishCmd(ch, std::static_pointer_cast<PublishCmd>(req->cmd), rsp);
        } else if (req->cmd->cmdType == watch_cmd) {
            
        } else if (req->cmd->cmdType == delete_queue_cmd) {
            threadDelQueueCmd(ch, std::static_pointer_cast<DeleteQueueCmd>(req->cmd), rsp);
        } else if (req->cmd->cmdType == delete_exchange_cmd) {
            threadDelExchangeCmd(ch, std::static_pointer_cast<DeleteExchangeCmd>(req->cmd), rsp);
        } else if (req->cmd->cmdType == get_cmd) {
            threadGetCmd(ch, std::static_pointer_cast<GetCmd>(req->cmd), rsp);
        } else if (req->cmd->cmdType == ack_cmd) {
            threadAckCmd(ch, std::static_pointer_cast<AckCmd>(req->cmd), rsp);
        }

        auto gData = globalData();
        gData->lock.lock();

        auto& pool = gData->corePool[ch->id];
        if (checkConn(rsp->reply)) {
            pool.valid.push_back(ch->core);
            if (!checkChannel(rsp->reply)) {
                threadCloseChannel(ch->core, ch->chId);
            }
        }

        gData->lock.unlock();
    } while (false);

    tData->lock.lock();
    // 存入队列
    tData->rspQueue.push_back(rsp);
    tData->lock.unlock();
}

void threadWatcherPoll(RabbitWatcher& watcher, bool& changed, bool& idle) {
    while (true) {
        RabbitRspDataPtr rsp = std::make_shared<RabbitRspData>();
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 10; //1000 * 2;

        amqp_maybe_release_buffers(watcher.core->conn);
        rsp->reply = amqp_consume_message(watcher.core->conn, &rsp->envelope, &tv, 0);
                
        if (AMQP_RESPONSE_NORMAL == rsp->reply.reply_type) {
            // 收到一个完整的消息
            //rabbitLog("recv channel: %d", rsp->envelope.channel);
            for (auto iter = watcher.cbs.begin(); iter != watcher.cbs.end(); iter++) {
                auto& info = iter->second;
                if (info.chId == rsp->envelope.channel) {
                    idle = false;
                    // 放入队列
                    rsp->watch_cb = info.cb;
                    RabbitThreadData* tData = (RabbitThreadData*)info.tData;
                    tData->lock.lock();
                    // 存入队列
                    tData->rspQueue.push_back(rsp);
                    tData->lock.unlock();
                    break;
                }
            }
            continue;
        } else if (AMQP_RESPONSE_LIBRARY_EXCEPTION == rsp->reply.reply_type) {
            if (rsp->reply.library_error == AMQP_STATUS_TIMEOUT) {
                // 超时
                // rabbitLog("timeout%s", "");
                break;
            } 
        }

        // error
        changed = true;
        watcher.core = nullptr;
        watcher.genChId = 1;
        for (auto iter = watcher.cbs.begin(); iter != watcher.cbs.end(); iter++) {
            iter->second.chId = 0;
        }
        break;
    }
}

void threadWatcherCreate(std::unordered_map<std::string, RabbitWatcher>& watchPool, bool& changed, bool& idle) {
    if (watchPool.empty()) {
        return;
    }

    time_t curTime = 0;
    time(&curTime);

    for (auto iter = watchPool.begin(); iter != watchPool.end(); iter++) {
        auto& watcher = iter->second;
        if (!watcher.core) {
            if (curTime - watcher.lastCreate >= 5) {
                changed = true;
                watcher.lastCreate = curTime;
                RabbitAddrPtr addr = std::make_shared<RabbitAddr>(iter->first);
                watcher.core = threadCreateCore(addr);
                if (!watcher.core) {
                    rabbitLog("[error] failed to connect for watch: %s", iter->first.c_str());
                }
            } else {
                //rabbitLog("time limit....%s", "");
            }
        }
        if (!watcher.core) {
            continue;
        }

        bool poll = false;
        for (auto iter2 = watcher.cbs.begin(); iter2 != watcher.cbs.end(); iter2++) {
            auto& info = iter2->second;
            if (info.cancel) {
                if (info.chId != 0) {
                    // 关闭channel
                    changed = true;
                    rabbitLog("close channel: %s, %d", iter->first.c_str(), info.chId);
                    amqp_basic_cancel(watcher.core->conn, info.chId, amqp_cstring_bytes(info.cmd->consumer_tag.c_str()));
                    amqp_maybe_release_buffers_on_channel(watcher.core->conn, info.chId);
                    checkError(amqp_channel_close(watcher.core->conn, info.chId, AMQP_CHANNEL_ERROR), 
                        "amqp_channel_close"
                    );
                }
            } else if (info.chId == 0 && curTime - info.lastCreate >= 5) {
                changed = true;
                info.lastCreate = curTime;
                uint32_t chId = watcher.genChId++;
                auto ch = threadCreateChannel(watcher.core, chId);
                if (!ch) {
                    rabbitLog("[error] failed to open channel for watch: %s, %d", iter->first, chId);
                    goto c_err;
                }

                // 创建consumer
                RabbitRspDataPtr rsp = std::make_shared<RabbitRspData>();
                threadWatch(ch, info.cmd, rsp);
                if (rsp->ok) {
                    info.chId = chId;
                } else if (!checkConn(rsp->reply)) {
                    goto c_err;
                } else {
                    // 关闭channel
                    threadCloseChannel(watcher.core, chId);
                }
            }

            if (info.chId != 0) {
                poll = true;
            }
        } 

        if (poll) {
            threadWatcherPoll(watcher, changed, idle);
        }
        continue;

c_err:
        watcher.core = nullptr;
        watcher.genChId = 1;
        for (auto iter2 = watcher.cbs.begin(); iter2 != watcher.cbs.end(); iter2++) {
            iter2->second.chId = 0;
        }
    }
}

void threadWriateBackWatcher(std::unordered_map<std::string, RabbitWatcher>& src, std::unordered_map<std::string, RabbitWatcher>& other) {
    for (auto iter1 = src.begin(); iter1 != src.end(); ) {
        auto iter2 = other.find(iter1->first);
        if (iter2 != other.end()) {
            iter1->second = iter2->second;
        }

        for (auto iter3 = iter1->second.cbs.begin(); iter3 != iter1->second.cbs.end(); ) {
            if (iter3->second.cancel) {
                iter3 = iter1->second.cbs.erase(iter3);
            } else {
                iter3++;
            }
        }

        if (iter1->second.cbs.empty()) {
            iter1 = src.erase(iter1);
        } else {
            iter1++;
        }
    }
}

void threadWatcherProcess(GlobalData* gData) {
    std::unordered_map<std::string, RabbitWatcher> watchPool;

    gData->watchLock.lock();
    watchPool = gData->watchPool;
    gData->watchLock.unlock();

    bool changed = false;
    bool idle = true;

    threadWatcherCreate(watchPool, changed, idle);

    if (idle) {
        gData->watchIdle++;
    } else {
        gData->watchIdle = 0;
    }

    gData->watchIdle++;

    if (changed) {
        // 回写watchPool
        gData->watchLock.lock();
        threadWriateBackWatcher(gData->watchPool, watchPool);
        gData->watchLock.unlock();
    }

    // clear flag
    gData->watcherRun.clear();
}

std::function<void()> localPop(RabbitThreadData* tData) {
    if (tData->reqQueue.empty()) {
        assert(false);
        return std::function<void()>();
    }

    auto req = tData->reqQueue.front();
    tData->reqQueue.pop_front();

    auto f = std::bind(threadProcess, req, tData);
    return f;
}

void localRespond(RabbitThreadData* tData) {
    if (tData->rspQueue.empty()) {
        return;
    }

    // 交换队列
    std::list<RabbitRspDataPtr> rspQueue;
    tData->lock.lock();
    rspQueue.swap(tData->rspQueue);
    tData->lock.unlock();

    for (auto iter = rspQueue.begin(); iter != rspQueue.end(); iter++) {
        auto rsp = *iter;
        if (rsp->watch_cb) {
            char* body = (char *)rsp->envelope.message.body.bytes;
            size_t len = rsp->envelope.message.body.len;
            rsp->watch_cb(&rsp->reply, &rsp->envelope, rsp->envelope.delivery_tag, body, len);
        } else if (rsp->req->get_cb) {
            char* body = (char *)rsp->message.body.bytes;
            size_t len = rsp->message.body.len;
            rsp->req->get_cb(&rsp->reply, &rsp->message, rsp->ok, body, len);
        } else {
            tData->rspTaskCnt++;
            rsp->req->cb(&rsp->reply, rsp->ok);
        }
        amqp_destroy_envelope(&rsp->envelope);
        amqp_destroy_message(&rsp->message);
    }
}

void localProcess(uint32_t curTime, RabbitThreadData* tData) {
     if (tData->reqQueue.empty()) {
        return;
    }

    if (gIoThr) {
        gIoThr(localPop(tData));
    } else {
        // 调用线程担任io线程的角色
        localPop(tData)();
    }
}

void localStatistics(int32_t curTime, RabbitThreadData* tData) {
    if (curTime - tData->lastPrintTime < 120) {
        return;
    }

    tData->lastPrintTime = curTime;
    rabbitLog("[statistics] curTask: %d, reqTask: %d, rspTask: %d",
        tData->reqTaskCnt - tData->rspTaskCnt,
        tData->reqTaskCnt,
        tData->rspTaskCnt);

    auto gData = globalData();
    gData->lock.lock();
    for (auto& item : gData->corePool) {
        for (auto c : item.second.valid) {
            rabbitLog("[statistics] id: %s, conns: %d, ch: %d", item.first.c_str(), item.second.valid.size(), c->chId);
        }
    }
    gData->lock.unlock();

    gData->watchLock.lock();
    for (auto& item : gData->watchPool) {
        for (auto& item2 : item.second.cbs) {
            rabbitLog("[statistics] watcher id: %s, tag: %s, ch: %d", item.first.c_str(), item2.first.c_str(),  item2.second.chId);
        }
    }
    gData->watchLock.unlock();
}

clock_t getMilClock() {
#ifndef WIN32
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#else
	return GetTickCount();
#endif
}

bool localWatch(RabbitThreadData* tData) {
    auto gData = globalData();
    if (gData->watcherRun.test_and_set()) {
        return false;
    }
    
    do {
        if (gData->watchPool.empty()) {
            break;
        }

        clock_t nTick = getMilClock();
        if (nTick - gData->watchIdleTick < 100) {
            //rabbitLog("idle...%s", "");
            break;
        }

        // gData->watchIdleTick = 0;
        if (gData->watchIdle >= 500) {
            gData->watchIdleTick = nTick;
            gData->watchIdle = 0;
            break;
        }

        auto f = std::bind(threadWatcherProcess, gData);
        if (gIoThr) {
            gIoThr(f);
        } else {
            f();
        }

        return true;
    } while (false);

    gData->watcherRun.clear();
    return false;
}

////////////////////////////////////////////////////////////////////////////////////

// 设置最大通道数,超过最大通道数后，就会起新的连接
void setMaxChannel(uint32_t c) {
    //gMaxChannel = c;
}

void execute(const std::string& uri, std::shared_ptr<BaseRabbitCmd> cmd, async_rabbit_cb cb) {
    // 构造一个请求
    RabbitReqDataPtr req = std::make_shared<RabbitReqData>();
    req->cmd = cmd;
    req->addr = std::make_shared<RabbitAddr>(uri);
    req->cb = cb;
    time(&req->reqTime);

    RabbitThreadData* tData = runningData();
    tData->reqQueue.push_back(req);
    tData->reqTaskCnt++;
}

void execute(const std::string& uri, std::shared_ptr<GetCmd> cmd, async_rabbit_get_cb cb) {
    // 构造一个请求
    RabbitReqDataPtr req = std::make_shared<RabbitReqData>();
    req->cmd = cmd;
    req->addr = std::make_shared<RabbitAddr>(uri);
    req->get_cb = cb;
    time(&req->reqTime);

    RabbitThreadData* tData = runningData();
    tData->reqQueue.push_back(req);
    tData->reqTaskCnt++;
}

bool watch(const std::string& uri, std::shared_ptr<WatchCmd> cmd, async_rabbit_watch_cb cb) {
    if (cmd->queue.empty()) {
        return false;
    }
    
    bool ret = false;
    std::string watchId = cmd->queue + "#" + cmd->consumer_tag;

    auto gData = globalData();
    auto tData = runningData();
    
    // 注册
    gData->watchLock.lock();
    
    auto& wPool = gData->watchPool[uri];
    auto iterWatch = wPool.cbs.find(watchId);
    if (iterWatch == wPool.cbs.end()) {
        auto& watchInfo = wPool.cbs[watchId];
        watchInfo.tData = tData;
        watchInfo.cb = cb;
        watchInfo.cmd = cmd;
    }
    
    gData->watchLock.unlock();
    return ret;
}

void unwatch(const std::string& uri, std::shared_ptr<UnWatchCmd> cmd) {
    if (cmd->queue.empty()) {
        return;
    }

    std::string watchId = cmd->queue + "#" + cmd->consumer_tag;

    auto gData = globalData();

    gData->watchLock.lock();
    
    auto iterPool = gData->watchPool.find(uri);
    if (iterPool != gData->watchPool.end()) {
        auto iterWatch = iterPool->second.cbs.find(watchId);
        if (iterWatch != iterPool->second.cbs.end()) {
            iterWatch->second.cancel = true;
        }
    }

    gData->watchLock.unlock();
}

bool loop(uint32_t curTime) {
    RabbitThreadData* tData = runningData();
    localStatistics(curTime, tData);
    localRespond(tData);
    localProcess(curTime, tData);
    bool w = localWatch(tData);

    bool hasTask = tData->reqTaskCnt != tData->rspTaskCnt;
    return hasTask || w;
}

void setThreadFunc(std::function<void(std::function<void()>)> f) {
    gIoThr = f;
}

}
}

#endif