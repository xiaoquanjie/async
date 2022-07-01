/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

/*
 * 
*/

#pragma once

#include <functional>
#include <memory>

namespace co_async {

// promise的返回值
enum {
    // 访问成功
    E_OK = 0,
    // 访问超时
    E_TIMEOUT = 1,
    // 访问失败, 
    E_ERROR = 2
};

struct PromiseReject {
    int err = 0;
    std::string msg;
};

struct PromiseResult {
    bool timeOutFlag = false;
    int64_t timerId = 0;
    int64_t uniqueId = 0;
    uint32_t coId = 0;
    uint32_t fns = 0;
    std::shared_ptr<void> resolveRes;
    std::shared_ptr<PromiseReject> rejectRes;
};

struct Resolve {
    std::shared_ptr<PromiseResult> result;
    void operator()(std::shared_ptr<void> res) const;
};

struct Reject {
    std::shared_ptr<PromiseResult> result;
    void operator()(int err, const std::string& msg) const;
};

// 循环，内部调用异步循环
bool loopPromise(uint32_t curTime = 0);

// 返回值是pair: first=[E_OK/E_TIMEOUT/E_ERROR], second=[std::shared_ptr<void>/nullptr/other]
// 1、first=E_OK时, second=目标
// 2、first=E_TIMEOUT时，second=nullptr
// 3、first=E_ERROR时，second=[std::shared_ptr<PromiseReject>/nullptr]
std::pair<int, std::shared_ptr<void>> promise(std::function<void(Resolve, Reject)> fn, int timeOut = 3000);

void setTimeout(std::function<void()> fn, int timeOut);

//////////////////////// // 辅助获取promise的值 

inline bool checkOk(std::pair<int, std::shared_ptr<void>> p) {
    return (p.first == E_OK);
}

inline bool checkTimeout(std::pair<int, std::shared_ptr<void>> p) {
    return (p.first == E_TIMEOUT);
}

inline bool checkError(std::pair<int, std::shared_ptr<void>> p) {
    return (p.first == E_ERROR);
}

template<typename T>
inline std::shared_ptr<T> getOk(std::pair<int, std::shared_ptr<void>> p) {
    if (checkOk(p)) {
        auto res = std::static_pointer_cast<T>(p.second);
        return res;
    }
    else {
        return nullptr;
    }
}

inline std::shared_ptr<PromiseReject> getError(std::pair<int, std::shared_ptr<void>> p) {
    if (checkError(p)) {
        auto res = std::static_pointer_cast<PromiseReject>(p.second);
        return res;
    }
    else {
        return nullptr;
    }
}

//////////////////////// // 辅助获取promise的值 

}



