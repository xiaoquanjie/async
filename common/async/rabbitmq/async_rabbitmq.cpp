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
#include "common/async/rabbitmq/op.h"
#include <rabbitmq-c/tcp_socket.h>

namespace async {

clock_t getMilClock();
uint32_t supposeIothread();

namespace rabbitmq {

#define IO_THREADS async::supposeIothread()

// io线程的函数
std::function<void(std::function<void()>)> gIoThr = nullptr;
// 超时时间
uint32_t gTimeout = 10;
// 挂载任务数
uint32_t gMountTask = 10;

/////////////////////////////////////////////////////////////////////////////////////////

RabbitThreadData* runningData() {
    thread_local static RabbitThreadData gThrData;
    return &gThrData;
}

GlobalData* globalData() {
    static GlobalData gData;
    return &gData;
}

/////////////////////////////////////////////////////////////////////////////////////////

RabbitCorePtr localCreateCore(RabbitAddrPtr addr) {
    auto core = std::make_shared<RabbitCore>();
    core->addr = addr;
    return core;
}

RabbitConnPtr threadCreateConn(RabbitCorePtr core) {
    auto conn = std::make_shared<RabbitConn>();
    conn->c = amqp_new_connection();

    amqp_socket_t *socket = amqp_tcp_socket_new(conn->c);
    if (!socket) {
        rabbitLog("[error] failed to call amqp_tcp_socket_new%s", "");
        return nullptr;
    }

    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    int rc = amqp_socket_open_noblock(socket, core->addr->host.c_str(), core->addr->port, &tv);
    if (rc != AMQP_STATUS_OK) {
       rabbitLog("[error] failed to call amqp_socket_open_noblock: %s|%d", core->addr->host.c_str(), core->addr->port);
       return nullptr;
    }

    amqp_rpc_reply_t rpc_reply = amqp_login(conn->c, 
        core->addr->vhost.c_str(), 
        0, 
        AMQP_DEFAULT_FRAME_SIZE,
        AMQP_DEFAULT_HEARTBEAT, 
        AMQP_SASL_METHOD_PLAIN, 
        core->addr->user.c_str(), 
        core->addr->pwd.c_str()
    );

    if (!checkError(rpc_reply, "amqp_login")) {
        return nullptr;
    }

    amqp_set_rpc_timeout(conn->c, &tv);
    rabbitLog("connect rabbitmq: %s successfully", core->addr->id.c_str());
    
    conn->id = core->addr->id;
    return conn;
}

uint32_t threadCreateChannel(RabbitConnPtr conn) {
    if (!conn) {
        return 0;
    }

    uint32_t id = conn->genChId++;
    amqp_channel_open(conn->c, id);
    if (checkError(amqp_get_rpc_reply(conn->c), "amqp_channel_open")) {
        rabbitLog("open channel: %s, %d successfully", conn->id.c_str(), id);
        conn->chIds.insert(id);
        return id;
    }
    else {
        return 0;
    }
}

// 关闭channel
void threadCloseChannel(RabbitConnPtr conn, uint32_t chId) {
    if (conn->chIds.find(chId) == conn->chIds.end()) {
        assert(false);
        return;
    }

    rabbitLog("close channel: %s, %d", conn->id.c_str(), chId);
    amqp_channel_close_ok_t close_ok;
    amqp_send_method(conn->c, chId, AMQP_CHANNEL_CLOSE_OK_METHOD, &close_ok);
    amqp_maybe_release_buffers_on_channel(conn->c, chId);
    conn->chIds.erase(chId);
}

bool threadWatch(RabbitConnPtr conn, WatcherPtr w) {
    if (w->chId == 0) {
        return true;
    }

    amqp_basic_consume(conn->c, 
        w->chId, 
        amqp_cstring_bytes(w->cmd->queue.c_str()), 
        amqp_cstring_bytes(w->cmd->consumer_tag.c_str()), 
        w->cmd->no_local ? 1 : 0, 
        w->cmd->no_ack ? 1 : 0, 
        w->cmd->exclusive ? 1 : 0,
        amqp_empty_table
    );

    auto reply = amqp_get_rpc_reply(conn->c);
    if (checkError(reply, "amqp_basic_consume")) {
        rabbitLog("watch queue:%s, consumer_tag: %s in %s, %d", 
            w->cmd->queue.c_str(), 
            w->cmd->consumer_tag.c_str(),
            conn->id.c_str(),
            w->chId);
    } else {
        if (checkConn(reply)) {
            return false;
        }
        else {
            // 关闭频道
            threadCloseChannel(conn, w->chId);
            w->chId = 0;
        }
    }

    return true;
}

void threadProcess(std::list<RabbitCorePtr> coreList) {
    time_t curTime =0;
    time(&curTime);

    for (auto c : coreList) {
        if (!c->conn) {
            c->conn = threadCreateConn(c);
        }

        // 不可长时间占用cpu
        int count = 5;
        std::list<RabbitReqDataPtr> waitQueue;
        std::list<WatcherPtr> watchers;
        c->waitLock.lock();
        for (auto iter = c->waitQueue.begin(); iter != c->waitQueue.end();) {
            if (count == 0) {
                break;
            }
            count--;
            waitQueue.push_back(*iter);
            iter = c->waitQueue.erase(iter);
        }
        watchers = c->watchers;
        c->waitLock.unlock();

        for (auto req : waitQueue) {
            uint32_t chId = 0;
            if (c->conn) {
                if (c->conn->chIds.empty()) {
                    chId = threadCreateChannel(c->conn);
                }
                else {
                    chId = *(c->conn->chIds.begin());
                }
            }
            opCmd(c, chId, req);
        }

        if (c->conn && !watchers.empty()) {
            for (auto w : watchers) {
                if (w->chId == 0) {
                    w->chId = threadCreateChannel(c->conn);
                    if (!threadWatch(c->conn, w)) {
                        goto watch_err;
                    }
                }
                else {
                    std::list<RabbitReqDataPtr> acks;
                    w->aLock.lock();
                    acks.swap(w->acks);
                    w->aLock.unlock();
                    bool ackRet = true;
                    for (auto req : acks) {
                        if (!opCmd(c, w->chId, req)) {
                            ackRet = false;
                        }
                    }
                    if (!ackRet) {
                        goto watch_err;
                    }
                }
            }
            //rabbitLog("enter opwatch%s", "");
            if (!opWatch(c)) {
                goto watch_err;
            }
            //rabbitLog("leave opwatch%s", "");
            continue;
watch_err:
            c->conn = 0;
            for (auto w : watchers) {
                w->chId = 0;
                std::list<RabbitReqDataPtr> acks;
                w->aLock.lock();
                acks.swap(w->acks);
                w->aLock.unlock();
                for (auto ack : acks) {
                    opAckError(ack);
                }
            }
        }   
    }

    // 去掉运行标识
    for (auto c : coreList) {
        c->running = false;
    }    
}

void localProcess(uint32_t curTime, RabbitThreadData* tData) {
    if (tData->reqQueue.empty()) {
        return;
    }

    uint32_t ioThreads = IO_THREADS;
    auto gData = globalData();

    gData->pLock.lock();
    for (auto iterQueue = tData->reqQueue.begin(); iterQueue != tData->reqQueue.end(); iterQueue++) {
        auto req = *iterQueue;
        auto pool = gData->getPool(req->addr->id);

        if (req->cmd->cmdType == ack_cmd) {
            if (pool->wCore) {
                auto cmd = std::static_pointer_cast<AckCmd>(req->cmd);
                std::string watchId = cmd->queue + "#" + cmd->consumer_tag;
                auto iterW = pool->watchers.find(watchId);
                if (iterW != pool->watchers.end()) {
                    iterW->second->aLock.lock();
                    iterW->second->acks.push_back(req);
                    iterW->second->aLock.unlock();
                }
            }
            continue;
        }

        // 遍历一个有效的core
        RabbitCorePtr core;
        if (ioThreads == pool->valid.size()) {
            core = pool->valid[pool->polling++ % ioThreads];
        }
        else {
            for (auto c : pool->valid) {
                if (c->task < gMountTask) {
                    core = c;
                    break;
                }
            }

            if (!core) {
                // 建新core
                if (pool->valid.size() < ioThreads) {
                    core = localCreateCore(req->addr);
                    pool->valid.push_back(core);
                }
            }
        }

        core->waitLock.lock();
        core->waitQueue.push_back(req);
        core->waitLock.unlock();
        core->task++;
    }
    tData->reqQueue.clear();
    gData->pLock.unlock();
}

void localDispatchTask(RabbitThreadData* tData) {
    auto gData = globalData();
    // 抢占到分配标识
    if (gData->dispatchTask.test_and_set()) {
        return;
    }

    // 把所有的core分为IO_THREADS组
    uint32_t count = IO_THREADS;
    std::vector<std::list<RabbitCorePtr>> coreGroup(count);
    uint32_t groupIdx = 0;

    // 加锁
    gData->pLock.lock();
    for (auto& poolItem : gData->corePool) {
        auto pool = poolItem.second;
        // 监听
        for (auto& wm : pool->watchers) {
            auto w = wm.second;
            if (w->dispatch) {
                continue;
            }
            if (!pool->wCore) {
                auto addr = std::make_shared<RabbitAddr>(poolItem.first);
                pool->wCore = localCreateCore(addr);
            }
            pool->wCore->waitLock.lock();
            pool->wCore->watchers.push_back(w);
            pool->wCore->waitLock.unlock();
            w->dispatch = true;
        }

        if (pool->wCore && !(pool->wCore->running) && pool->wCore->watchers.size() > 0) {
            pool->wCore->running = true;
            coreGroup[(groupIdx++) % count].push_back(pool->wCore);
        }

        for (auto c : pool->valid) {
            if (c->running) {
                continue;
            }
            if (c->task == 0) {
                continue;
            }
           
            c->running = true;
            c->lastRunTime = 0;
            coreGroup[(groupIdx++) % count].push_back(c);
        }
    }
    // 解锁
    gData->pLock.unlock();

    // 清标记
    gData->dispatchTask.clear();    

    for (size_t idx = 0; idx < coreGroup.size(); idx++) {
        auto& g = coreGroup[idx];
        if (g.empty()) {
            continue;
        }
        
        // for test
        // for (auto c : g) {
        //     rabbitLog("dispatch task to group: %d and core_nums: %d, task: %d, core: %p", idx, g.size(), c->task.load(), c.get());
        // }
        
        if (gIoThr) {
            std::function<void()> f = std::bind(threadProcess, g);
            gIoThr(f);
        } else {
            threadProcess(g);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

void localRespond(RabbitThreadData* tData) {
    if (tData->rspQueue.empty()) {
        return;
    }

    // 交换队列
    std::list<RabbitRspDataPtr> rspQueue;
    tData->rspLock.lock();
    rspQueue.swap(tData->rspQueue);
    tData->rspLock.unlock();

    for (auto iter = rspQueue.begin(); iter != rspQueue.end(); iter++) {
        auto rsp = *iter;
        if (rsp->req->addr && rsp->req->cmd->cmdType != ack_cmd) {
            tData->rspTaskCnt++;
        }
        rsp->req->cb(rsp->parser);
    }
}

void localStatistics(int32_t curTime, RabbitThreadData* tData) {
    if (curTime - tData->lastPrintTime < 40) {
        return;
    }

    tData->lastPrintTime = curTime;
    rabbitLog("[statistics] curTask: %d, reqTask: %d, rspTask: %d",
        tData->reqTaskCnt - tData->rspTaskCnt,
        tData->reqTaskCnt,
        tData->rspTaskCnt);

    auto gData = globalData();
    gData->pLock.lock();
    for (auto& item : gData->corePool) {
        for (auto c : item.second->valid) {
            uint32_t chs = c->conn ? c->conn->chIds.size() : 0;
            rabbitLog("[statistics] id: %s, conns: %d, core: %p, chs: %d", item.first.c_str(), item.second->valid.size(), c.get(), chs);
        }
        rabbitLog("[statistics] id: %s, watchers: %d", item.first.c_str(), item.second->watchers.size());
    }
    gData->pLock.unlock();
}

/////////////////////////////////////////////////////////////////////////////////////////

void execute(const std::string& uri, std::shared_ptr<BaseRabbitCmd> cmd, async_rabbit_cb cb) {
    auto tData = runningData();
    tData->init = true;

    auto req = std::make_shared<RabbitReqData>();
    req->cb = cb;
    req->cmd = cmd;
    req->addr = std::make_shared<RabbitAddr>(uri);
    req->tData = tData;
    time(&req->reqTime);

    tData->reqQueue.push_back(req);
    tData->reqTaskCnt++;
}

bool watch(const std::string& uri, std::shared_ptr<WatchCmd> cmd, async_rabbit_cb cb) {
    auto tData = runningData();
    tData->init = true;

    if (cmd->queue.empty()) {
        return false;
    }
    
    bool ret = false;
    std::string watchId = cmd->queue + "#" + cmd->consumer_tag;

    auto gData = globalData();
    
    // 注册
    gData->pLock.lock();
    
    auto pool = gData->getPool(uri);
    auto iterWatch = pool->watchers.find(watchId);
    if (iterWatch == pool->watchers.end()) {
        auto w = std::make_shared<Watcher>();
        w->cb = cb;
        w->cmd = cmd;
        w->tData = tData;
        pool->watchers[watchId] = w;
        ret = true;
    }
    
    gData->pLock.unlock();
    return ret;
}

void unwatch(const std::string& uri, std::shared_ptr<UnWatchCmd> cmd) {
    if (cmd->queue.empty()) {
        return;
    }

    std::string watchId = cmd->queue + "#" + cmd->consumer_tag;
    auto gData = globalData();

    gData->pLock.lock();
    auto pool = gData->getPool(uri);
    auto iterWatch = pool->watchers.find(watchId);
    if (iterWatch != pool->watchers.end()) {
        iterWatch->second->cancel = true;
        pool->watchers.erase(iterWatch);
    }
    gData->pLock.unlock();
}

bool watchAck(const std::string& uri, std::shared_ptr<AckCmd> cmd, async_rabbit_cb cb) {
    if (cmd->queue.empty()) {
        return false;
    }

    auto tData = runningData();
    tData->init = true;

    auto req = std::make_shared<RabbitReqData>();
    req->cb = cb;
    req->cmd = cmd;
    req->addr = std::make_shared<RabbitAddr>(uri);
    req->tData = tData;
    time(&req->reqTime);

    tData->reqQueue.push_back(req);
    return true;
}

bool loop(uint32_t curTime) {
    auto tData = runningData();
    if (!tData->init) {
        return false;
    }

    localStatistics(curTime, tData);
    localRespond(tData);
    localProcess(curTime, tData);
    localDispatchTask(tData);
    
    bool hasTask = tData->reqTaskCnt != tData->rspTaskCnt;
    return hasTask;
}

void setThreadFunc(std::function<void(std::function<void()>)> f) {
    gIoThr = f;
}

}
}

#endif
