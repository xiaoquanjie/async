/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#include "common/ipc/ipc.h"
#include <stdarg.h>

namespace ipc {

std::function<void(const char*)> g_log_cb = [](const char* data) {
    printf("[zeromq] %s\n", data);
};

void log(const char* format, ...) {
    if (!g_log_cb) {
        return;
    }

    char buf[1024] = { 0 };
    va_list ap;
    va_start(ap, format);
    vsprintf(buf, format, ap);
    g_log_cb(buf);
}

void setLogFunc(std::function<void(const char*)> cb) {
    g_log_cb = cb;
}

}