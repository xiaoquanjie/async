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
#include <atomic>
#include <mutex>
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
    std::shared_ptr<BaseRedisCmd> cmd;
    RedisAddrPtr addr;
    time_t reqTime = 0;
    void* tData = 0;
};

typedef std::shared_ptr<RedisReqData> RedisReqDataPtr;

// 回复
struct RedisRspData {
    RedisReplyParserPtr parser;
    RedisReqDataPtr req;
};

typedef std::shared_ptr<RedisRspData> RedisRspDataPtr;

struct RedisConn {
    void *ctxt = 0;
    uint32_t state = enum_connecting_state;
    bool cluster = false;
    std::string id;
    bool activeClosed = true;
    ~RedisConn();
};

typedef std::shared_ptr<RedisConn> RedisConnPtr;

// redis连接核心
struct RedisCore {
    RedisConnPtr conn;
    RedisAddrPtr addr;
    time_t lastConnTime = 0;
    std::atomic<uint32_t> task;
    std::list<RedisReqDataPtr> waitQueue;
    std::mutex waitLock;
    ~RedisCore();
};

typedef std::shared_ptr<RedisCore> RedisCorePtr;

struct CorePool {
    // 事件核心
    event_base* evtBase = 0;
    bool running = false;
    clock_t lastRunTime = 0;
    std::unordered_map<std::string, RedisCorePtr> coreMap;
    uint32_t getTask();
};

typedef std::shared_ptr<CorePool> CorePoolPtr;

struct GlobalData {
    // 连接池
    uint32_t polling = 0;
    std::vector<CorePoolPtr> corePool;
    std::mutex pLock;

    // 分发标识 
    std::atomic_flag dispatchTask;

    GlobalData() :dispatchTask(ATOMIC_FLAG_INIT) {}
};

struct RedisThreadData {
    // 请求任务的数量
    uint64_t reqTaskCnt = 0;
    // 回复任务的数量 
    uint64_t rspTaskCnt = 0;
    // 请求队列
    std::list<RedisReqDataPtr> reqQueue;
    // 回复队列
    std::mutex rspLock;
    std::list<RedisRspDataPtr> rspQueue;
    uint32_t lastRunTime = 0;
    // 上一次统计输出时间
    time_t lastPrintTime = 0;
    // 初始化
    bool init = false;
};

struct InvokeData {
    RedisReqDataPtr req;
    RedisCorePtr core;
};

///////////////////////////////////////////////////////////////////////////////

#define redisLog(format, ...) log("[async_redis] " format, __VA_ARGS__)

redisReply* copyRedisReply(redisReply* reply);

void convertCmd(const BaseRedisCmd& cmd, std::vector<const char *>& argv, std::vector<size_t>& argc);

}
}
