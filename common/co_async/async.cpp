/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#include "common/co_async/async.h"
#include "common/co_async/promise.h"
#include "common/async/async.h"
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
    bool ret = loopPromise(curTime);
    ret = ret | async::loop(curTime);
    return ret;
}

}