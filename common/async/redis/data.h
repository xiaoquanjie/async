/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include <vector>
#include <list>
#include <unordered_map>
#include <assert.h>
#include <functional>
#include "common/async/redis/async_redis.h"
#include "common/log.h"

struct redisReply;
struct event_base;

namespace async {
namespace redis {

// redis连接状态
enum {
    enum_connecting_state = 0,
    enum_connected_state,
    enum_authing_state,
    enum_auth_state,        // 已认证通过
    enum_selecting_state,
    enum_selected_state,
    enum_disconnected_state,
};

// op type
enum {
    enum_op_auth = 0,
    enum_op_select,
    enum_op_result,
};

// 地址结构
struct RedisAddr {
    std::string id;
    std::string host; // 集群：host1:port1,host2:port2 非集群:host
    uint32_t port = 0;
    std::string pwd; // 密码
    uint32_t dbIdx = 0;
    bool cluster = false;

    RedisAddr();

    RedisAddr(const std::string& id);

    void parse(const std::string& id); 
};

typedef std::shared_ptr<RedisAddr> RedisAddrPtr;

// 请求
struct RedisReqData {
    async_redis_cb cb;
    BaseRedisCmd cmd;
    RedisAddrPtr addr;
    time_t reqTime = 0;
};

typedef std::shared_ptr<RedisReqData> RedisReqDataPtr;

// 回复
struct RedisRspData {
    redisReply* reply = 0;
    RedisReqDataPtr req;
};

typedef std::shared_ptr<RedisRspData> RedisRspDataPtr;

// redis连接核心
struct RedisCore {
    void *ctxt = 0;
    RedisAddrPtr addr;
    uint32_t state = enum_connecting_state;
    RedisReqDataPtr curReq;
    bool passiveClose = false;
    ~RedisCore();
};

typedef std::shared_ptr<RedisCore> RedisCorePtr;

struct CorePool {
    time_t lastCreate = 0;            // 上一次创建core的时间
    time_t lastDisconnect = 0;        // 上一次断联时间
    std::list<RedisCorePtr> valid;      // 可用的
    std::list<RedisCorePtr> conning;    // 正在连接的队列
    std::unordered_map<void*, RedisCorePtr>  useing;   // 正在使用中的, using是关键字
};

struct RedisThreadData {
    // 事件核心
    event_base* evtBase = 0;
    // 请求任务的数量
    uint64_t reqTaskCnt = 0;
    // 回复任务的数量 
    uint64_t rspTaskCnt = 0;
    // 请求队列
    std::list<RedisReqDataPtr> reqQueue;
    std::list<RedisReqDataPtr> prepareQueue;
    // 回复队列
    std::list<RedisRspDataPtr> rspQueue;
    // 连接池
    std::unordered_map<std::string, CorePool> corePool;
    // 标记
    bool running = false;
    uint32_t lastRunTime = 0;
    // 上一次统计输出时间
    time_t lastPrintTime = 0;
};

///////////////////////////////////////////////////////////////////////////////

#define redisLog(format, ...) log("[async_redis] " format, __VA_ARGS__)

redisReply* copyRedisReply(redisReply* reply);

void convertCmd(const BaseRedisCmd& cmd, std::vector<const char *>& argv, std::vector<size_t>& argc);

}
}
