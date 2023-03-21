/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
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

RabbitCore::~RabbitCore() {
    // 关闭channel
    if (this->chId != 0) {
        rabbitLog("close channel: %s, %d", addr->id.c_str(), this->chId);
        amqp_maybe_release_buffers_on_channel(this->conn, this->chId);
        amqp_channel_close(this->conn, this->chId, AMQP_REPLY_SUCCESS);
    }
    if (this->conn) {
        rabbitLog("close connection: %s", addr->id.c_str());
        amqp_maybe_release_buffers(this->conn);
        amqp_connection_close(this->conn, AMQP_REPLY_SUCCESS);
        amqp_destroy_connection(this->conn);
    }
}


}
}

#endif