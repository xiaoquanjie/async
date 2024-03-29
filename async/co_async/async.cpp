/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#include "async/co_async/async.h"
#include "async/co_async/promise.h"
#include "async/async/async.h"
#include <vector>
#include <assert.h>

namespace co_async { 

//////////////////////////////////////////////////////////////////////////

std::pair<int, int64_t> parallel(const std::initializer_list<fn_cb>& fns, int timeOut) {
    auto res = promise([fns](Resolve resolve, Reject reject) {
        async::parallel([resolve](int64_t result) {
            resolve(std::make_shared<int64_t>(result));
        }, fns);
    }, timeOut);

    std::pair<int, int64_t> ret = std::make_pair(res.first, 0);
    if (checkOk(res)) {
        auto p = getOk<int64_t>(res);
        ret.second = *p;
    }

    return ret;
}

/////////////////////////////////////////////////////////////////////////

bool loop(uint32_t curTime) {
    if (curTime == 0) {
       time((time_t*)&curTime);
    }
    bool ret1 = loopPromise(curTime);
    bool ret2 = async::loop(curTime);
    return ret1 || ret2;
}

// 带有sleep功能，在没有任务时降低cpu使用率
void loopSleep(uint32_t curTime, uint32_t sleepMil) {
    if (curTime == 0) {
       time((time_t*)&curTime);
    }

    loopPromise(curTime);
    async::loopSleep(curTime, sleepMil);
}

}