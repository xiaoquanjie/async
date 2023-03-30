/*----------------------------------------------------------------
// Copyright 2023
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_RABBITMQ

#include "common/async/rabbitmq/data.h"

namespace async {
namespace rabbitmq {

void onPushRsp(RabbitRspDataPtr rsp, void* t) {
    RabbitThreadData* tData = (RabbitThreadData*)t;
    tData->rspLock.lock();
    tData->rspQueue.push_back(rsp);
    tData->rspLock.unlock();
}

void onTimeout(RabbitReqDataPtr req) {
    auto rsp = std::make_shared<RabbitRspData>();
    rsp->req = req;
    rsp->parser = std::make_shared<RabbitParser>();
    rsp->parser->timeout = true;
    onPushRsp(rsp, req->tData);
}

amqp_rpc_reply_t onExchangeCmd(RabbitCorePtr c, uint32_t chId, RabbitReqDataPtr req) {
    auto cmd = std::static_pointer_cast<DeclareExchangeCmd>(req->cmd);
    auto rsp = std::make_shared<RabbitRspData>();
    rsp->req = req;
    rsp->parser = std::make_shared<RabbitParser>();

    amqp_exchange_declare(c->conn->c, 
        chId, 
        amqp_cstring_bytes(cmd->name.c_str()),
        amqp_cstring_bytes(cmd->type.c_str()), 
        cmd->passive ? 1 : 0, /*passive*/
        cmd->durable ? 1 : 0, /*durable*/
        cmd->autoDelete ? 1 : 0, /*auto_delete*/
        0,  /*internal*/
        amqp_empty_table
    );

    *((amqp_rpc_reply_t*)rsp->parser->reply) = amqp_get_rpc_reply(c->conn->c);
    checkError(*((amqp_rpc_reply_t*)rsp->parser->reply), "amqp_exchange_declare");
    onPushRsp(rsp, req->tData);
    return *((amqp_rpc_reply_t*)rsp->parser->reply);
}

amqp_rpc_reply_t onQueueCmd(RabbitCorePtr c, uint32_t chId, RabbitReqDataPtr req) {
    auto cmd = std::static_pointer_cast<DeclareQueueCmd>(req->cmd);
    auto rsp = std::make_shared<RabbitRspData>();
    rsp->req = req;
    rsp->parser = std::make_shared<RabbitParser>();

    amqp_queue_declare(c->conn->c,
        chId,
        amqp_cstring_bytes(cmd->name.c_str()),
        cmd->passive ? 1 : 0, /*passive*/
        cmd->durable ? 1 : 0, /*durable*/
        cmd->exclusive ? 1 : 0, /*durable*/
        cmd->autoDelete ? 1 : 0, /*auto_delete*/
        amqp_empty_table
    );

    *((amqp_rpc_reply_t*)rsp->parser->reply) = amqp_get_rpc_reply(c->conn->c);
    checkError(*((amqp_rpc_reply_t*)rsp->parser->reply), "amqp_queue_declare");
    onPushRsp(rsp, req->tData);
    return *((amqp_rpc_reply_t*)rsp->parser->reply);
}

amqp_rpc_reply_t onBindCmd(RabbitCorePtr c, uint32_t chId, RabbitReqDataPtr req) {
    auto cmd = std::static_pointer_cast<BindCmd>(req->cmd);
    auto rsp = std::make_shared<RabbitRspData>();
    rsp->req = req;
    rsp->parser = std::make_shared<RabbitParser>();

    amqp_queue_bind(c->conn->c, 
        chId, 
        amqp_cstring_bytes(cmd->queue.c_str()),
        amqp_cstring_bytes(cmd->exchange.c_str()), 
        amqp_cstring_bytes(cmd->routingKey.c_str()),
        amqp_empty_table
    );

    *((amqp_rpc_reply_t*)rsp->parser->reply) = amqp_get_rpc_reply(c->conn->c);
    checkError(*((amqp_rpc_reply_t*)rsp->parser->reply), "amqp_queue_bind");
    onPushRsp(rsp, req->tData);
    return *((amqp_rpc_reply_t*)rsp->parser->reply);
}

amqp_rpc_reply_t onUnbind(RabbitCorePtr c, uint32_t chId, RabbitReqDataPtr req) {
    auto cmd = std::static_pointer_cast<UnbindCmd>(req->cmd);
    auto rsp = std::make_shared<RabbitRspData>();
    rsp->req = req;
    rsp->parser = std::make_shared<RabbitParser>();

    amqp_queue_unbind(c->conn->c, 
        chId, 
        amqp_cstring_bytes(cmd->queue.c_str()),
        amqp_cstring_bytes(cmd->exchange.c_str()),
        amqp_cstring_bytes(cmd->routingKey.c_str()), 
        amqp_empty_table
    );

    *((amqp_rpc_reply_t*)rsp->parser->reply) = amqp_get_rpc_reply(c->conn->c);
    checkError(*((amqp_rpc_reply_t*)rsp->parser->reply), "amqp_queue_unbind");
    onPushRsp(rsp, req->tData);
    return *((amqp_rpc_reply_t*)rsp->parser->reply);
}

amqp_rpc_reply_t onPublish(RabbitCorePtr c, uint32_t chId, RabbitReqDataPtr req) {
    auto cmd = std::static_pointer_cast<PublishCmd>(req->cmd);
    auto rsp = std::make_shared<RabbitRspData>();
    rsp->req = req;
    rsp->parser = std::make_shared<RabbitParser>();

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

    // 默认是永久性消息
    if (!(props._flags & AMQP_BASIC_DELIVERY_MODE_FLAG)) {
        props._flags |= AMQP_BASIC_DELIVERY_MODE_FLAG;
        props.delivery_mode = 2; /* persistent delivery mode */
    }
    
    amqp_basic_publish(c->conn->c, 
        chId,
        amqp_cstring_bytes(cmd->exchange.c_str()),
        amqp_cstring_bytes(cmd->routingKey.c_str()), 
        cmd->mandatory ? 1 : 0,
        cmd->immediate ? 1 : 0, 
        &props,
        amqp_cstring_bytes(cmd->msg.c_str())
    );

    *((amqp_rpc_reply_t*)rsp->parser->reply) = amqp_get_rpc_reply(c->conn->c);
    checkError(*((amqp_rpc_reply_t*)rsp->parser->reply), "amqp_basic_publish");
    onPushRsp(rsp, req->tData);
    return *((amqp_rpc_reply_t*)rsp->parser->reply);
}

amqp_rpc_reply_t onDelQueueCmd(RabbitCorePtr c, uint32_t chId, RabbitReqDataPtr req) {
    auto cmd = std::static_pointer_cast<DeleteQueueCmd>(req->cmd);
    auto rsp = std::make_shared<RabbitRspData>();
    rsp->req = req;
    rsp->parser = std::make_shared<RabbitParser>();

    amqp_queue_delete(c->conn->c, 
        chId, 
        amqp_cstring_bytes(cmd->name.c_str()), 
        cmd->if_unused ? 1 : 0, 
        cmd->if_empty ? 1 : 0
    );

    *((amqp_rpc_reply_t*)rsp->parser->reply) = amqp_get_rpc_reply(c->conn->c);
    checkError(*((amqp_rpc_reply_t*)rsp->parser->reply), "amqp_queue_delete");
    onPushRsp(rsp, req->tData);
    return *((amqp_rpc_reply_t*)rsp->parser->reply);
}

amqp_rpc_reply_t onDelExchangeCmd(RabbitCorePtr c, uint32_t chId, RabbitReqDataPtr req) {
    auto cmd = std::static_pointer_cast<DeleteExchangeCmd>(req->cmd);
    auto rsp = std::make_shared<RabbitRspData>();
    rsp->req = req;
    rsp->parser = std::make_shared<RabbitParser>();

    amqp_exchange_delete(c->conn->c, 
        chId,
        amqp_cstring_bytes(cmd->name.c_str()), 
        cmd->if_unused ? 1 : 0
    );

    *((amqp_rpc_reply_t*)rsp->parser->reply) = amqp_get_rpc_reply(c->conn->c);
    checkError(*((amqp_rpc_reply_t*)rsp->parser->reply), "amqp_exchange_delete");
    onPushRsp(rsp, req->tData);
    return *((amqp_rpc_reply_t*)rsp->parser->reply);
}

amqp_rpc_reply_t onGetCmd(RabbitCorePtr c, uint32_t chId, RabbitReqDataPtr req) {
    auto cmd = std::static_pointer_cast<GetCmd>(req->cmd);
    auto rsp = std::make_shared<RabbitRspData>();
    rsp->req = req;
    rsp->parser = std::make_shared<RabbitParser>();

    auto reply = amqp_basic_get(c->conn->c, 
        chId, 
        amqp_cstring_bytes(cmd->queue.c_str()), 
        cmd->no_ack ? 1 : 0
    );

    if (checkError(reply, "amqp_basic_get")) {
        if (reply.reply.id != AMQP_BASIC_GET_EMPTY_METHOD) {
            rsp->parser->message = malloc(sizeof(amqp_message_t));
            reply = amqp_read_message(c->conn->c, chId, (amqp_message_t*)rsp->parser->message, 0);
            checkError(reply, "amqp_read_message");
        }
    }

    *((amqp_rpc_reply_t*)rsp->parser->reply) = reply;
    onPushRsp(rsp, req->tData);
    return *((amqp_rpc_reply_t*)rsp->parser->reply);
}

bool onAckCmd(RabbitCorePtr c, uint32_t chId, RabbitReqDataPtr req) {
    auto cmd = std::static_pointer_cast<AckCmd>(req->cmd);
    auto rsp = std::make_shared<RabbitRspData>();
    rsp->req = req;
    rsp->parser = std::make_shared<RabbitParser>();

    rsp->parser->x = amqp_basic_ack(c->conn->c, 
        chId, 
        cmd->delivery_tag, 
        0
    );

    if (rsp->parser->x != 0) {
        std::string context = "amqp_basic_ack in queue:" 
            + cmd->queue 
            + ", consumer_tag: " 
            + cmd->consumer_tag 
            + " in " 
            + c->addr->id
            + ", " 
            + std::to_string(chId);

        rabbitLog("[error] failed to call %s: %s", context.c_str(), amqp_error_string2(rsp->parser->x));
    }

    onPushRsp(rsp, req->tData);
    return rsp->parser->isOk();
}

void threadCloseChannel(RabbitConnPtr conn, uint32_t chId);

void opAckError(RabbitReqDataPtr req) {
    auto rsp = std::make_shared<RabbitRspData>();
    rsp->req = req;
    rsp->parser = std::make_shared<RabbitParser>();
    amqp_rpc_reply_t reply;
    reply.reply_type = AMQP_RESPONSE_LIBRARY_EXCEPTION;
    reply.library_error = AMQP_STATUS_CONNECTION_CLOSED;
    *((amqp_rpc_reply_t*)rsp->parser->reply) = reply;
    onPushRsp(rsp, req->tData);
}

bool opCmd(RabbitCorePtr c, uint32_t chId, RabbitReqDataPtr req) {
    amqp_rpc_reply_t reply;
    reply.reply_type = AMQP_RESPONSE_NORMAL;
    do {
        if (!c->conn || chId == 0) {
            reply.reply_type = AMQP_RESPONSE_LIBRARY_EXCEPTION;
            reply.library_error = AMQP_STATUS_CONNECTION_CLOSED;
            opAckError(req);
            break;
        }

        if (req->cmd->cmdType == declare_exchange_cmd) {
            reply = onExchangeCmd(c, chId, req);
        }
        else if (req->cmd->cmdType == declare_queue_cmd) {
            reply = onQueueCmd(c, chId, req);
        }
        else if (req->cmd->cmdType == bind_cmd) {
            reply = onBindCmd(c, chId, req);
        }
        else if (req->cmd->cmdType == unbind_cmd) {
            reply = onUnbind(c, chId, req);
        }
        else if (req->cmd->cmdType == publish_cmd) {
            reply = onPublish(c, chId, req);
        }
        else if (req->cmd->cmdType == delete_queue_cmd) {
            reply = onDelQueueCmd(c, chId, req);
        }
        else if (req->cmd->cmdType == delete_exchange_cmd) {
            reply = onDelExchangeCmd(c, chId, req);
        }
        else if (req->cmd->cmdType == get_cmd) {
            reply = onGetCmd(c, chId, req);
        }
        else if (req->cmd->cmdType == ack_cmd) {
            if (!onAckCmd(c, chId, req)) {
                reply = amqp_get_rpc_reply(c->conn->c);
            }
        }
    } while (false);

    if (req->cmd->cmdType != ack_cmd) {
        c->task--;
    }
    
    if (checkConn(reply)) {
        if (!checkChannel(reply)) {
            threadCloseChannel(c->conn, chId);
        }
    }
    else {
        c->conn = nullptr;
    }

    return reply.reply_type == AMQP_RESPONSE_NORMAL;
}

bool opWatch(RabbitCorePtr c) {
    for (;;) {
        auto parser = std::make_shared<RabbitParser>();
        parser->envelope = malloc(sizeof(amqp_envelope_t));
        amqp_envelope_t* envelope = (amqp_envelope_t*)parser->envelope;

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 10; //1000 * 2;

        amqp_maybe_release_buffers(c->conn->c);
        amqp_rpc_reply_t reply = amqp_consume_message(c->conn->c, envelope, &tv, 0);
        *((amqp_rpc_reply_t*)parser->reply) = reply;

        if (AMQP_RESPONSE_NORMAL == reply.reply_type) {
            // 收到一个完整的消息
            for (auto w : c->watchers) {
                if (w->chId == envelope->channel) {
                    RabbitRspDataPtr rsp = std::make_shared<RabbitRspData>();
                    rsp->parser = parser;
                    rsp->req = std::make_shared<RabbitReqData>();
                    rsp->req->cb = w->cb;
                    rsp->req->tData = w->tData;
                    onPushRsp(rsp, w->tData);
                    break;
                }
            }
            continue;
        }
        else if (AMQP_RESPONSE_LIBRARY_EXCEPTION == reply.reply_type) {
            if (reply.library_error == AMQP_STATUS_TIMEOUT) {
                // 超时
                // rabbitLog("timeout%s", "");
                break;
            } 
        }
        else {
            checkError(reply, "amqp_consume_message");
        }

        return false;
    }

    return true;
}

}
}

#endif