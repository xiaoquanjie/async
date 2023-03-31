/*----------------------------------------------------------------
// Copyright 2023
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_ZOOKEEPER

#include "async/co_async/zookeeper/co_zookeeper.h"
#include "async/co_async/promise.h"

namespace co_async {
namespace zookeeper {

std::pair<int, async::zookeeper::ZookParserPtr> execute(const std::string& uri, std::shared_ptr<async::zookeeper::BaseZookCmd> cmd, const TimeOut& t) {
    auto res = co_async::promise([&uri, cmd](co_async::Resolve resolve, co_async::Reject reject) {
        async::zookeeper::execute(uri, cmd, [resolve](async::zookeeper::ZookParserPtr parser) {
            resolve(parser);
        });
    }, t());

    std::pair<int, async::zookeeper::ZookParserPtr> ret = std::make_pair(res.first, nullptr);
    if (co_async::checkOk(res)) {
        ret.second = co_async::getOk<async::zookeeper::ZookParser>(res);
    } 
    return ret;
}

int execute(const std::string& uri, std::shared_ptr<async::zookeeper::BaseZookCmd> cmd, bool& ok, const TimeOut& t) {
    ok = false;
    auto res = co_async::promise([&uri, cmd](co_async::Resolve resolve, co_async::Reject reject) {
        async::zookeeper::execute(uri, cmd, [resolve](async::zookeeper::ZookParserPtr parser) {
            resolve(parser);
        });
    }, t());

    if (co_async::checkOk(res)) {
        auto parser = co_async::getOk<async::zookeeper::ZookParser>(res);
        ok = parser->zOk();
    }

    return res.first;
}

std::pair<int, bool> execute2(const std::string& uri, std::shared_ptr<async::zookeeper::BaseZookCmd> cmd, const TimeOut& t) {
    auto res = execute(uri, cmd, t);
    std::pair<int, bool> ret = std::make_pair(res.first, false);

    if (co_async::checkOk(res)) {
        auto parser = res.second;
        ret.second = parser->zOk();
    }

    return ret;
}

}
}
#endif