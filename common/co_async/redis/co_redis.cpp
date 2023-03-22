/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_REDIS

#include "common/co_async/redis/co_redis.h"
#include "common/async/redis/redis_exception.h"

namespace co_async {
namespace redis {

/////////////////////////////////////////////////////////////////////////

std::pair<int, async::redis::RedisReplyParserPtr> execute(const std::string& uri, const async::redis::BaseRedisCmd& cmd, const TimeOut& t) {
    auto res = co_async::promise([&uri, &cmd](co_async::Resolve resolve, co_async::Reject reject) {
        async::redis::execute(uri, cmd, [resolve](async::redis::RedisReplyParserPtr parser) {
            resolve(parser);
        });
    }, t());

    std::pair<int, async::redis::RedisReplyParserPtr> ret = std::make_pair(res.first, nullptr);
    if (co_async::checkOk(res)) {
        ret.second = co_async::getOk<async::redis::RedisReplyParser>(res);
    }
    else {
        log("[co_async_redis] failed to execute redis, timeout or error|%s|%d", cmd.GetCmd().c_str(), ret.first);
    }

    return ret;
}

std::pair<int, bool> execute(const std::string& uri, const async::redis::BaseRedisCmd& cmd, long long& result, const TimeOut& t) {
    auto res = execute(uri, cmd, t);
    std::pair<int, bool> ret = std::make_pair(res.first, false);
    
    if (co_async::checkOk(res)) {
        try {
            auto parser = res.second;
            parser->GetInteger(result);
            ret.second = true;
        }
        catch(async::redis::RedisException& exce) {
            log("[co_async_redis] failed to execute redis|%s|%s", cmd.GetCmd().c_str(), exce.What().c_str());
        }
    }

    return ret;
}

std::pair<int, bool> execute(const std::string& uri, const async::redis::BaseRedisCmd& cmd, std::string& result, const TimeOut& t) {
    auto res = execute(uri, cmd, t);
    std::pair<int, bool> ret = std::make_pair(res.first, false);

    if (co_async::checkOk(res)) {
        try {
            auto parser = res.second;
            parser->GetString(result);
            ret.second = true;
        }
        catch(async::redis::RedisException& exce) {
            log("[co_async_redis] failed to execute redis|%s|%s", cmd.GetCmd().c_str(), exce.What().c_str());
        }
    }
    
    return ret;
}

std::pair<int, bool> execute(const std::string& uri, const async::redis::BaseRedisCmd& cmd, bool& result, const TimeOut& t) {
    auto res = execute(uri, cmd, t);
    std::pair<int, bool> ret = std::make_pair(res.first, false);

    if (co_async::checkOk(res)) {
        try {
            auto parser = res.second;
            parser->GetOk(result);
            ret.second = true;
        }
        catch(async::redis::RedisException& exce) {
            log("[co_async_redis] failed to execute redis|%s|%s", cmd.GetCmd().c_str(), exce.What().c_str());
        }
    }
    
    return ret;
}

}
}

#endif