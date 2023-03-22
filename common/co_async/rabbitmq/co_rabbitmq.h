/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include "common/async/rabbitmq/async_rabbitmq.h"
#include "common/co_async/comm.hpp"

namespace co_async {
namespace rabbitmq {

// pair.second表示执行结果
std::pair<int, bool> execute(const std::string& uri, std::shared_ptr<async::rabbitmq::BaseRabbitCmd> cmd, const TimeOut& t = TimeOut());


}
}