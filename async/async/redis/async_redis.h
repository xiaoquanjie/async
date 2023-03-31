/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include <functional>
#include "async/async/redis/redis_parser.h"
#include "async/async/redis/redis_cmd.h"

namespace async {
namespace redis {

struct BaseRedisCmd;

typedef std::function<void(RedisReplyParserPtr)> async_redis_cb;

// @uri: [host|port|pwd|idx|cluster]
void execute(std::string uri, const BaseRedisCmd& redis_cmd, async_redis_cb cb);

void execute(std::string uri, std::shared_ptr<BaseRedisCmd> redis_cmd, async_redis_cb cb);

bool loop(uint32_t curTime);

void setThreadFunc(std::function<void(std::function<void()>)>);

} // redis
} // async