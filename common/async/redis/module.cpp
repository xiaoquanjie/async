/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_REDIS

#include "common/async/comm.hpp"
#include "common/async/redis/async_redis.h"

// 注册模块
static async::AsyncModule mod = {
    // 名字
    "async_cpu",
    // 主循环钩子
    std::bind(async::redis::loop, std::placeholders::_1),
    // 日志钩子
    std::bind(async::redis::setLogFunc, std::placeholders::_1),
    // 线程钩子
    std::bind(async::redis::setThreadFunc, std::placeholders::_1)
};

REG_ASYNC_MODULE(mod);

#endif