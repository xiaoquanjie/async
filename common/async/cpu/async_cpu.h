/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include <functional>

namespace async {
namespace cpu {

typedef std::function<int64_t(void*)> async_cpu_op;
typedef std::function<void(int64_t, void*)> async_cpu_cb;

void execute(async_cpu_op op, void* user_data, async_cpu_cb cb);

bool loop(uint32_t cur_time);

void setThreadFunc(std::function<void(std::function<void()>)>);

// 设置日志接口, 要求是线程安全的
void setLogFunc(std::function<void(const char*)> cb);

void log(const char* format, ...);

}
}