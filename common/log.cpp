/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#include "common/log.h"
#include <stdarg.h>
#include <mutex>

// 日志输出接口，是线程安全的
std::function<void(const char*)> gLogCb = [](const char* data) {
    static std::mutex sMutex;
    sMutex.lock();
    printf("%s\n", data);
    sMutex.unlock();
};

void log(const char* format, ...) {
    if (!gLogCb) {
        return;
    }

    char buf[1024] = { 0 };
    va_list ap;
    va_start(ap, format);
    vsprintf(buf, format, ap);
    gLogCb(buf);
}