/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#include "common/async/async.h"
#include "common/async/comm.hpp"
#include <vector>
#include <thread>
#include <time.h>

namespace async {

std::vector<AsyncModule>& getModuleVec() {
    static std::vector<AsyncModule> s_mod_vec;
    return s_mod_vec;
}

bool regModule(const AsyncModule& mod) {
    printf("reg async module:%s\n", mod.name.c_str());
    getModuleVec().push_back(mod);
    return true;
}

////////////////////////////////////////////////////////////////////////////////////

bool loop(uint32_t cur_time) {
    if (cur_time == 0) {
       // time(&cur_time);
    }

    auto& modVec = getModuleVec();
    bool is_busy = false;
    for (auto& item : modVec) {
        if (item.loopFunc) {
            if (item.loopFunc(cur_time)) {
                is_busy = true;
            }
        }
    }
    
    return is_busy;
}

void setThreadFunc(std::function<void(std::function<void()>)> cb) {
    auto& modVec = getModuleVec();
    for (auto& item : modVec) {
        if (item.thrFunc) {
            item.thrFunc(cb);
        }
    }
}   

static uint32_t gIoThread = 0;

// 假设最大的io线程数量
uint32_t supposeIothread() {
    if (gIoThread == 0) {
        // 假设为两倍的cpu核数量
        gIoThread = 1; // std::thread::hardware_concurrency() * 2;
    }
    return gIoThread;
}

// 设置io线程数量
void setIoThread(uint32_t c) {
    gIoThread = c >= 1 ? c : 1;
}

clock_t getMilClock() {
#ifndef WIN32
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#else
	return GetTickCount();
#endif
}

/////////////////////////////////////////////////

struct parallel_result {
    int fns = 0;
    int64_t result = 0;
};

void parallel(done_cb done, const std::initializer_list<fn_cb>& fns) {
    if (fns.size() == 0) {
        return;
    }

    auto result = std::make_shared<parallel_result>();
    result->fns = fns.size();
    
    next_cb next = [result, done](int64_t err) {
        if (err != 0) {
            result->result |= err;
        }
        result->fns -= 1;
        if (result->fns == 0) {
            done(result->result);
        }
    };

    for (auto& f : fns) {
        f(next);
    }
}

///////////////////////////////////////////////////////////////////
struct series_result {
    next_cb next;
    std::vector<fn_cb> fns;
    size_t idx = 0;
    int err = 0;
};

void series(done_cb done, const std::initializer_list<fn_cb>& fns) {
    if (fns.size() == 0) {
        return;
    }

    auto result = std::make_shared<series_result>();
    for (auto& f : fns) {
        result->fns.push_back(f);
    }

    next_cb next = [result, done](int err) {
        if (err != 0) {
            result->err |= err;
        }
        result->idx++;
        if (result->idx == result->fns.size()) {
            int e = result->err;
            done(e);
        }    
        else {
            result->fns[result->idx](result->next);
        }
    };
    
    result->next = next;
    result->fns[result->idx](next);
}

///////////////////////////////////////////////////////////////////

// 字符串分割函数
void split(const std::string source, 
    const std::string &separator, 
    std::vector<std::string> &array) {
    array.clear();
    std::string::size_type start = 0;
    while (true) {
        std::string::size_type pos = source.find(separator, start);
        if (pos == std::string::npos) {
            std::string sub = source.substr(start, source.size());
            array.push_back(sub);
            break;
        }

        std::string sub = source.substr(start, pos - start);
        start = pos + separator.size();
        array.push_back(sub);
    }
}

}