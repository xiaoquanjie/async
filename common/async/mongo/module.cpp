/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_MONGO

#include "common/async/comm.hpp"
#include "common/async/mongo/async_mongo.h"

// 注册模块
static async::AsyncModule mod = {
    // 名字
    "async_cpu",
    // 主循环钩子
    std::bind(async::mongo::loop, std::placeholders::_1),
    // 日志钩子
    std::bind(async::mongo::setLogFunc, std::placeholders::_1),
    // 线程钩子
    std::bind(async::mongo::setThreadFunc, std::placeholders::_1)
};

REG_ASYNC_MODULE(mod);

#endif