/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include <stdint.h>
#include <functional>
#include "async/co_async/comm.hpp"

namespace co_async {

typedef std::function<void(int64_t)> next_cb;
typedef std::function<void(next_cb)> fn_cb;

// 并行执行里面的所有异步操作
std::pair<int, int64_t> parallel(const std::initializer_list<fn_cb>& fns, int timeOut = 10 * 1000);

bool loop(uint32_t curTime = 0);

// 带有sleep功能，在没有任务时降低cpu使用率
void loopSleep(uint32_t curTime, uint32_t sleepMil = 0);

}

