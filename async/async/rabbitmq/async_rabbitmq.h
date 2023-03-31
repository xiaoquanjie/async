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
#include "async/async/rabbitmq/rabbitmq_cmd.h"
#include "async/async/rabbitmq/rabbitmq_parser.h"

namespace async {
namespace rabbitmq {

// reply: amqp_rpc_reply_t
typedef std::function<void(RabbitParserPtr parser)> async_rabbit_cb;

// @uri: [host|port|vhost|user|pwd]
void execute(const std::string& uri, std::shared_ptr<BaseRabbitCmd> cmd, async_rabbit_cb cb);

// 监听
bool watch(const std::string& uri, std::shared_ptr<WatchCmd> cmd, async_rabbit_cb cb);

// 取消监听
void unwatch(const std::string& uri, std::shared_ptr<UnWatchCmd> cmd);

// 监听ack
bool watchAck(const std::string& uri, std::shared_ptr<AckCmd> cmd, async_rabbit_cb cb);

bool loop(uint32_t curTime);

void setThreadFunc(std::function<void(std::function<void()>)> f);

// 控制watch的接收速率
void setConsumeRate(uint32_t rate);

} // rabbitmq
} // async