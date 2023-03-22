/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include <functional>
#include <memory>
#include <string>
#include "common/async/rabbitmq/rabbitmq_cmd.h"

namespace async {
namespace rabbitmq {

// 设置最大通道数,暂时不起作用
void setMaxChannel(uint32_t c);

// reply: amqp_rpc_reply_t
typedef std::function<void(void* reply, bool ok)> async_rabbit_cb;

// reply: amqp_rpc_reply_t
// envelope: amqp_envelope_t
// body: msg
typedef std::function<void(void* reply, void* envelope, char* body, size_t len)> async_rabbit_watch_cb;

// @uri: [host|port|vhost|user|pwd]
void execute(const std::string& uri, std::shared_ptr<BaseRabbitCmd> cmd, async_rabbit_cb cb);

// 监听
bool watch(const std::string& uri, std::shared_ptr<WatchCmd> cmd, async_rabbit_watch_cb);

// 取消监听
void unwatch(const std::string& uri, std::shared_ptr<UnWatchCmd> cmd);

bool loop(uint32_t curTime);

void setThreadFunc(std::function<void(std::function<void()>)> f);

} // rabbitmq
} // async