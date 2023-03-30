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
#include <list>

namespace co_async {
namespace rabbitmq {

struct rabbit_get_result {
    bool ok;
    std::string msg;
};

std::pair<int, async::rabbitmq::RabbitParserPtr> execute(const std::string& uri, std::shared_ptr<async::rabbitmq::BaseRabbitCmd> cmd, const TimeOut& t) {
    auto res = co_async::promise([&uri, cmd](co_async::Resolve resolve, co_async::Reject reject) {
        async::rabbitmq::execute(uri, cmd, [resolve](async::rabbitmq::RabbitParserPtr parser) {
            resolve(parser);
        });
    }, t());

    std::pair<int, async::rabbitmq::RabbitParserPtr> ret = std::make_pair(res.first, nullptr);
    if (co_async::checkOk(res)) {
        ret.second = co_async::getOk<async::rabbitmq::RabbitParser>(res);
    } 
    return ret;
}

std::pair<int, bool> execute2(const std::string& uri, std::shared_ptr<async::rabbitmq::BaseRabbitCmd> cmd, const TimeOut& t) {
    auto res = execute(uri, cmd, t);
    std::pair<int, bool> ret = std::make_pair(res.first, false);
    if (co_async::checkOk(res)) {
        auto parser = res.second;
    } 

    return ret;
}

std::pair<int, bool> watchAck(const std::string& uri, std::shared_ptr<async::rabbitmq::AckCmd> cmd, const TimeOut& t) {
    auto res = co_async::promise([&uri, cmd](co_async::Resolve resolve, co_async::Reject reject) {
        async::rabbitmq::watchAck(uri, cmd, [resolve](async::rabbitmq::RabbitParserPtr parser) {
            resolve(parser);
        });
    }, t());

    std::pair<int, bool> ret = std::make_pair(res.first, false);
    if (co_async::checkOk(res)) {
        auto parser = co_async::getOk<async::rabbitmq::RabbitParser>(res);
        ret.second = parser->isOk();
    } 
    return ret;
}

// bool watch(const std::string& uri, std::shared_ptr<async::rabbitmq::WatchCmd> cmd, co_async_rabbit_watch_cb cb) {
//     bool ret = async::rabbitmq::watch(uri, cmd, [cb](void* reply, void* envelope, uint64_t delivery_tag, char* body, size_t len) {
//         CoroutineTask::doTask([cb, reply, envelope, delivery_tag, body, len](void*) {
//             cb(reply, envelope, delivery_tag, body, len);
//         }, 0);
//     });
//     return ret;
// }

}
}

#endif