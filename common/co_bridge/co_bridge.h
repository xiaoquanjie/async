/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

/*
 * 协程io桥，提供各个协程io时需要调用的共同接口，算是一种协调者
*/

#pragma once

#include "common/co_async/co_io.h"
#include "common/co_async/cpu/co_cpu.h"
#include "common/co_async/curl/co_curl.h"
#include "common/co_async/mongo/co_mongo.h"
#include "common/co_async/redis/co_redis.h"
#include "common/co_async/mysql/co_mysql.h"
#include <stdint.h>
#include <functional>

namespace co_bridge {

// 协程io返回码
enum Co_Wait_Return {
    E_CO_RETURN_OK = 0,         // 访问成功
    E_CO_RETURN_TIMEOUT = 1,    // 访问超时
    E_CO_RETURN_ERROR = 2,      // 访问失败，几乎不存在这种错误，只有在非协程状态下调用协程接口才会导致该错误
};

// 等待时间
enum Wait_Time {
    E_WAIT_THREE_SECOND = 3 * 1000,
    E_WAIT_FIVE_SECOND = 5 * 1000,
    E_WAIT_TEN_SECOND = 10 * 1000,
	E_WAIT_THIRTY_SECOND = 30 * 1000,
    E_WAIT_SIXTY_SECOND = 60 * 1000,
    E_WAIT_TWO_MINUTE = 2 * 60 * 1000,
};

// 毫秒
int wait(uint32_t interval);

// 生成唯一id
int64_t genUniqueId();

void addUniqueId(int64_t id);

bool rmUniqueId(int64_t id);

// 生成唯一序列码
int64_t genSequenceId();

void addSequenceId(int64_t id, int64_t timer_id, unsigned int co_id, void* extra);

bool rmSequenceId(int64_t id, int64_t& timer_id, unsigned int& co_id, void** extra);

bool rmSequenceId(int64_t id);

int64_t addTimer(int interval, std::function<void()> func);

void rmTimer(int64_t timer_id);

// 循环，内部调用异步循环
bool loop();

}