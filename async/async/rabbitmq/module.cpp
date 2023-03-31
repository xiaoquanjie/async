/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_RABBITMQ

#include "async/async/comm.hpp"
#include "async/async/rabbitmq/async_rabbitmq.h"

// 注册模块
static async::AsyncModule mod = {
    // 名字
    "async_rabbitmq",
    // 主循环钩子
    std::bind(async::rabbitmq::loop, std::placeholders::_1),
    // 日志钩子
    nullptr,
    // 线程钩子
    std::bind(async::rabbitmq::setThreadFunc, std::placeholders::_1)
};

REG_ASYNC_MODULE(mod);

#endif