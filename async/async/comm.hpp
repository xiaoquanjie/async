/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include <functional>

namespace async {

// 模块结构
struct AsyncModule {
    std::string name;
    // 主循环钩子
    std::function<bool(uint32_t)> loopFunc;
    // 日志钩子
    std::function<void(std::function<void(const char*)>)> logFunc;
    // 线程钩子
    std::function<void(std::function<void(std::function<void()>)>)> thrFunc;
};

// 注册模块
bool regModule(const AsyncModule&);

}

// 注册异步模块
#define REG_ASYNC_MODULE(mod) \
static bool ret_##__LINE__ = async::regModule(mod);

