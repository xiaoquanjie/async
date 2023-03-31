/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_RABBITMQ

#include "common/async/rabbitmq/data.h"
#include <cassert>

namespace async {
namespace rabbitmq {

RabbitAddr::RabbitAddr(const std::string& id) {
    parse(id);
}

void RabbitAddr::parse(const std::string& id) {
    this->id = id;
    std::vector<std::string> values;
    async::split(id, "|", values);

    if (values.size() != 5) {
        rabbitLog("[error] failed to parse id: %s", id.c_str());
        assert(false);
        return;
    }

    this->host = values[0];
    this->port = atoi(values[1].c_str());
    this->vhost = values[2];
    this->user = values[3];
    this->pwd = values[4];
}

RabbitConn::~RabbitConn() {
    if (!this->id.empty()) {
        rabbitLog("%s close connection: %s", this->id.c_str());
    }
    
    // 关闭channel
    for (auto id : this->chIds) {
        amqp_maybe_release_buffers_on_channel(this->c, id);
        amqp_channel_close(this->c, id, AMQP_REPLY_SUCCESS);
    }
    if (this->c) {
        amqp_maybe_release_buffers(this->c);
        amqp_connection_close(this->c, AMQP_REPLY_SUCCESS);
        amqp_destroy_connection(this->c);
    }
}

RabbitCore::~RabbitCore() {
}

CorePoolPtr GlobalData::getPool(const std::string& id) {
    CorePoolPtr pool;
    auto iter = this->corePool.find(id);
    if (iter == this->corePool.end()) {
        pool = std::make_shared<CorePool>();
        this->corePool[id] = pool;
    } else {
        pool = iter->second;
    }
    return pool;
}

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

bool checkError(int x, char const* context) {
    if (x != AMQP_STATUS_OK) {
        rabbitLog("[error] failed to call %s: %s", context, amqp_error_string2(x));
        return false;
    }
    return true;
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

}
}

#endif