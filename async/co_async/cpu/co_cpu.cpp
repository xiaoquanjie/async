/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#include "async/co_async/cpu/co_cpu.h"
#include "async/co_async/promise.h"
namespace co_async {
namespace cpu {

/////////////////////////////////////////////////

std::pair<int, int64_t> execute(async::cpu::async_cpu_op op, void* userData, const TimeOut& t) {
    auto res = co_async::promise([op, userData](co_async::Resolve resolve, co_async::Reject) {
        async::cpu::execute(op, userData, [resolve](int64_t res, void*) {
            resolve(std::make_shared<int64_t>(res));
        });
    }, t());

    auto ret = std::make_pair(res.first, 0);
    if (co_async::checkOk(res)) {
        auto p = co_async::getOk<int64_t>(res);
        ret.second = *p;
    }
    
    return ret;
}

}
}