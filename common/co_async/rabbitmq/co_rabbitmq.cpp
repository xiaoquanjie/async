/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_RABBITMQ

#include "common/co_async/rabbitmq/co_rabbitmq.h"
#include "common/coroutine/coroutine.hpp"
#include "common/co_async/promise.h"

namespace co_async {
namespace rabbitmq {

std::pair<int, bool> execute(const std::string& uri, std::shared_ptr<async::rabbitmq::BaseRabbitCmd> cmd, const TimeOut& t) {
    auto res = co_async::promise([&uri, cmd](co_async::Resolve resolve, co_async::Reject reject) {
        async::rabbitmq::execute(uri, cmd, [resolve](void* reply, bool ok) {
            std::shared_ptr<bool> result = std::make_shared<bool>(ok);
            resolve(result);
        });
    }, t());

    std::pair<int, bool> ret = std::make_pair(res.first, false);
    if (co_async::checkOk(res)) {
        ret.second = *(co_async::getOk<bool>(res));
    } 
    // else {
    //     log("[co_async_rabbitmq] failed to execute rabbit, timeout or error|%d", ret.first);
    // }
    return ret;
}

}
}

#endif