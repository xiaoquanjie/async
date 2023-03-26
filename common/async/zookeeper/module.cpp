/*----------------------------------------------------------------
// Copyright 2023
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_ZOOKEEPER

#include "common/async/comm.hpp"
#include "common/async/zookeeper/async_zookeeper.h"

// 注册模块
static async::AsyncModule mod = {
    // 名字
    "async_zookeeper",
    // 主循环钩子
    std::bind(async::zookeeper::loop, std::placeholders::_1),
    // 日志钩子
    nullptr,
    // 线程钩子
    std::bind(async::zookeeper::setThreadFunc, std::placeholders::_1)
};

REG_ASYNC_MODULE(mod);

#endif