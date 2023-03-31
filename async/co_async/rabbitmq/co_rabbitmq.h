/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include "async/async/rabbitmq/async_rabbitmq.h"
#include "async/co_async/comm.hpp"

namespace co_async {
namespace rabbitmq {

std::pair<int, async::rabbitmq::RabbitParserPtr> execute(const std::string& uri, std::shared_ptr<async::rabbitmq::BaseRabbitCmd> cmd, const TimeOut& t = TimeOut());

// pair.second表示执行结果
std::pair<int, bool> execute2(const std::string& uri, std::shared_ptr<async::rabbitmq::BaseRabbitCmd> cmd, const TimeOut& t = TimeOut());

std::pair<int, bool> watchAck(const std::string& uri, std::shared_ptr<async::rabbitmq::AckCmd> cmd, const TimeOut& t = TimeOut());

// 当有队列消息被监听到，cb会在协程里被调用
// warn: 此函数不能在子协程里被调用，要在主协程里被调用 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//bool watch(const std::string& uri, std::shared_ptr<async::rabbitmq::WatchCmd> cmd, co_async_rabbit_watch_cb cb);

}
}