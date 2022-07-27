/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include "common/async/redis/async_redis.h"
#include "common/co_async/promise.h"
#include "common/log.h"
#include <map>

/**
 * notice：因为async::redis::execute内部有链接失败重试机制(10秒)，所以co_async::redis::execute的超时时间最好不要短于10秒
*/

namespace co_async {
namespace redis {

std::pair<int, async::redis::RedisReplyParserPtr> execute(const std::string& uri, const async::redis::BaseRedisCmd& cmd, const TimeOut& t = TimeOut());

// @返回值pair.second表示redis语句操作是否成功
std::pair<int, bool> execute(const std::string& uri, const async::redis::BaseRedisCmd& cmd, long long& result, const TimeOut& t = TimeOut());

std::pair<int, bool> execute(const std::string& uri, const async::redis::BaseRedisCmd& cmd, std::string& result, const TimeOut& t = TimeOut());

std::pair<int, bool> execute(const std::string& uri, const async::redis::BaseRedisCmd& cmd, bool& result, const TimeOut& t = TimeOut());

// get array
template<typename T>
std::pair<int, bool> execute(const std::string& uri, const async::redis::BaseRedisCmd& cmd, T& result, const TimeOut& t = TimeOut()) {
    auto res = execute(uri, cmd, t);
    std::pair<int, bool> ret = std::make_pair(res.first, false);
    
    if (co_async::checkOk(res)) {
        try {
            auto parser = res.second;
			parser->GetArray(result);
            ret.second = true;
		}
		catch (const async::redis::RedisException &exce) {
			log("[co_async_redis] failed to execute redis|%s|%s\n", cmd.GetCmd().c_str(), exce.What().c_str());
		}
    }
    
    return ret;    
}

// get map
template<typename T1, typename T2>
std::pair<int, bool> execute(const std::string& uri, const async::redis::BaseRedisCmd& cmd, std::map<T1, T2>& result, const TimeOut& t = TimeOut()) {
    auto res = execute(uri, cmd, t);
    std::pair<int, bool> ret = std::make_pair(res.first, false);

    if (co_async::checkOk(res)) {
        try {
            auto parser = res.second;
			parser->GetMap(result);
            ret.second = true;
		}
		catch (const async::redis::RedisException &exce) {
			log("[co_async_redis] failed to execute redis|%s|%s\n", cmd.GetCmd().c_str(), exce.What().c_str());
		}
    }

    return ret;
}

// get scan result
template<typename T>
std::pair<int, bool> execute(const std::string& uri, const async::redis::BaseRedisCmd& cmd, long long& cursor, T& result, const TimeOut& t = TimeOut()) {
    auto res = execute(uri, cmd, t);
    std::pair<int, bool> ret = std::make_pair(res.first, false);

    if (co_async::checkOk(res)) {
        try {
            auto parser = res.second;
			parser->GetScan(cursor, result);
            ret.second = true;
		}
		catch (const async::redis::RedisException &exce) {
			log("[co_async_redis] failed to execute redis|%s|%s\n", cmd.GetCmd().c_str(), exce.What().c_str());
		}
    }
    
    return ret;
}

} // redis
} // co_async