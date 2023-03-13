/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include <functional>
#include "common/async/redis/redis_parser.h"
#include "common/async/redis/redis_cmd.h"

namespace async {
namespace redis {

struct BaseRedisCmd;

typedef std::function<void(RedisReplyParserPtr)> async_redis_cb;

// @uri: [host|port|pwd|idx|cluster]
/*hiredis_vip不支持带认证的集群*/
void execute(std::string uri, const BaseRedisCmd& redis_cmd, async_redis_cb cb);

// 同步redis接口, 无连接池
bool executeSync(std::string uri, const BaseRedisCmd& redis_cmd, async_redis_cb cb);

bool loop(uint32_t cur_time);

void setThreadFunc(std::function<void(std::function<void()>)>);

} // redis
} // async