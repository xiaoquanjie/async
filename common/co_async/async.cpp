/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#include "common/co_async/async.h"
#include "common/coroutine/coroutine.hpp"
#include "common/async/async.h"
#include "common/co_async/promise.h"
#include <vector>
#include <assert.h>

namespace co_async { 

// 前置声明
int64_t genUniqueId();
void addUniqueId(int64_t id);
int64_t addTimer(int interval, std::function<void()> func);
bool rmUniqueId(int64_t id);
void rmTimer(int64_t timerId);

//////////////////////////////////////////////////////////////////////////

int parallel(const std::initializer_list<fn_cb>& fns, int timeOut) {
    if (fns.size() == 0) {
        return E_OK;
    }

    unsigned int coId = Coroutine::curid();
    if (coId == M_MAIN_COROUTINE_ID) {
        assert(false);
        return E_ERROR;
    }

    // 生成一个唯一id
    int64_t uniqueId = genUniqueId();

    // 添加唯一值进去
    addUniqueId(uniqueId);

    auto result = std::make_shared<PromiseResult>();
    result->coId = coId;
    result->fns = fns.size();
    result->uniqueId = uniqueId;

    // 完成
    auto done = [result] () {
        if (!rmUniqueId(result->uniqueId)) {
            printf("done\n");
            return;
        }

        if (result->timerId != 0) {
            rmTimer(result->timerId);
            Coroutine::resume(result->coId);
        }
    };

    // 下一步
    next_cb next = [result, done]() {
        result->fns -= 1;
        if (result->fns == 0) {
            done();
        }
    };

    // 执行
    for (auto& f : fns) {
        f(next);
    }

    // 如果立马有了结果，则说明没有异步
    if (result->fns != 0) {
        // 添加定时器
        auto timerId = addTimer(timeOut, [result]() {
            if (!rmUniqueId(result->uniqueId)) {
                printf("timeout\n");
                return;
            }
            result->timeOutFlag = true;
            Coroutine::resume(result->coId);
        });
        
        result->timerId = timerId;
        // 挂起协程
        Coroutine::yield();
    }
    
    int ret = E_OK;
    if (result->timeOutFlag) {
        ret = E_TIMEOUT;
    }
    
    return ret;
}

/////////////////////////////////////////////////////////////////////////

bool loop(uint32_t curTime) {
    loopPromise(curTime);
    return async::loop(curTime);
}

}