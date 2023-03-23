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

struct rabbit_get_result {
    bool ok;
    std::string msg;
};

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
    return ret;
}

std::pair<int, bool> execute(const std::string& uri, std::shared_ptr<async::rabbitmq::GetCmd> cmd, std::string& msg, const TimeOut& t) {
    auto res = co_async::promise([&uri, cmd, &msg](co_async::Resolve resolve, co_async::Reject reject) {
        async::rabbitmq::execute(uri, cmd, [resolve](void* reply, void* message, bool ok, char* body, size_t len) {
            std::shared_ptr<rabbit_get_result> result = std::make_shared<rabbit_get_result>();
            result->ok = ok;
            if (body && len > 0) {
                result->msg.append(body, len);
            }
            resolve(result);
        });
    }, t());

    std::pair<int, bool> ret = std::make_pair(res.first, false);
    if (co_async::checkOk(res)) {
        auto result = co_async::getOk<rabbit_get_result>(res);
        ret.second = result->ok;
        msg.swap(result->msg);
    } 
    return ret;
}

std::pair<int, bool> watchAck(const std::string& uri, std::shared_ptr<async::rabbitmq::AckCmd> cmd, const TimeOut& t) {
    auto res = co_async::promise([&uri, cmd](co_async::Resolve resolve, co_async::Reject reject) {
        bool x = async::rabbitmq::watchAck(uri, cmd, [resolve](bool ok) {
            std::shared_ptr<bool> result = std::make_shared<bool>(ok);
            resolve(result);
        });
        if (!x) {
            std::shared_ptr<bool> result = std::make_shared<bool>(false);
            resolve(result);
        }
    }, t());

    std::pair<int, bool> ret = std::make_pair(res.first, false);
    if (co_async::checkOk(res)) {
        ret.second = *(co_async::getOk<bool>(res));
    } 
    return ret;
}

}
}

#endif