/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include <functional>

void log(const char* format, ...);

#ifdef DEBUG
#define logDebug log
#else
#define logDebug(...)
#endif

#ifdef TRACE
#define logTrace log
#else
#define logTrace(...)
#endif

// 设置日志接口, 要求是线程安全的
void setSafeLogFunc(std::function<void(const char*)> cb);