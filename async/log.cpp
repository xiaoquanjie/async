/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#include "async/log.h"
#include <stdarg.h>
#include <mutex>
#include <thread>
#include <sstream>
#include <time.h>
#include <iomanip>

#ifndef WIN32
#include <sys/time.h>
#endif

std::string curThreadId() {
    std::stringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
}

// 日志输出接口，是线程安全的
std::function<void(const char*)> gLogCb = [](const char* data) {
#ifdef WIN32    
    auto tp = std::chrono::system_clock::now();
    time_t t = std::chrono::system_clock::to_time_t(tp);
    std::stringstream ss;
    ss << std::put_time(localtime(&t), "%Y-%m-%d %H-%M-%S");
#else
    struct timeval tv;
    gettimeofday(&tv,0);

    time_t now = tv.tv_sec;
    struct tm lt;
    localtime_r(&now, &lt);

    char buf[50];
    int len = strftime(buf, 50, "%Y-%m-%d %H:%M:%S", &lt);
    len += snprintf(buf + len, 50 - len, ":%03d", (int)(tv.tv_usec/1000));
#endif

    static std::mutex sMutex;
    sMutex.lock();
#ifdef WIN32
    printf("[%s] [%s] %s\n", curThreadId().c_str(), ss.str().c_str(), data);
#else
    printf("[%s] [%s] %s\n", curThreadId().c_str(), buf, data);
#endif
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

void setSafeLogFunc(std::function<void(const char*)> cb) {
    gLogCb = cb;
}