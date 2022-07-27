/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include <functional>

void log(const char* format, ...);

// 设置日志接口, 要求是线程安全的
void setLogFunc(std::function<void(const char*)> cb);