/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include <functional>
#include <memory>
#include <list>
#include <mutex>
#include <vector>
#include <atomic>
#include <set>
#include <rabbitmq-c/amqp.h>
#include "async/async/rabbitmq/async_rabbitmq.h"
#include "async/log.h"

namespace async {

// 声明
void split(const std::string source, const std::string &separator, std::vector<std::string> &array);

namespace rabbitmq {

// 地址结构
struct RabbitAddr {
    std::string id;
    std::string host;
    uint32_t port = 0;
    std::string vhost;
    std::string user;
    std::string pwd;

    RabbitAddr(const std::string& id);

    void parse(const std::string& id);
};

typedef std::shared_ptr<RabbitAddr> RabbitAddrPtr;

// 请求
struct RabbitReqData {
    async_rabbit_cb cb;
    std::shared_ptr<BaseRabbitCmd> cmd;
    RabbitAddrPtr addr;
    time_t reqTime = 0;
    void* tData = 0;
};

typedef std::shared_ptr<RabbitReqData> RabbitReqDataPtr;

// 回复
struct RabbitRspData {
    RabbitReqDataPtr req;
    RabbitParserPtr  parser;
};

typedef std::shared_ptr<RabbitRspData> RabbitRspDataPtr;

struct RabbitConn {
    std::string id;
    amqp_connection_state_t c = 0;
    uint32_t genChId = 1;
    std::set<uint32_t> chIds;
    ~RabbitConn();
};

typedef std::shared_ptr<RabbitConn> RabbitConnPtr;

struct Watcher {
    bool dispatch = false;
    std::shared_ptr<WatchCmd> cmd;
    async_rabbit_cb cb;
    bool cancel = false;
    std::list<RabbitReqDataPtr> acks;
    std::mutex aLock;
    void* tData = 0;
    uint32_t chId = 0;
};

typedef std::shared_ptr<Watcher> WatcherPtr;

struct RabbitCore {
    RabbitConnPtr conn;
    RabbitAddrPtr addr;
    std::atomic<uint32_t> task;
    bool running = false;
    clock_t lastRunTime = 0;
    std::list<RabbitReqDataPtr> waitQueue;
    std::list<WatcherPtr> watchers;
    std::mutex waitLock;
    ~RabbitCore();
};

typedef std::shared_ptr<RabbitCore> RabbitCorePtr;

struct CorePool {
    uint32_t polling = 0;
    // 有用的连接
    std::vector<RabbitCorePtr> valid;   
    
    // 监听者
    std::unordered_map<std::string, WatcherPtr> watchers;
    RabbitCorePtr wCore;
};

typedef std::shared_ptr<CorePool> CorePoolPtr;

struct GlobalData {
    // 连接池
    std::unordered_map<std::string, CorePoolPtr> corePool;
    std::mutex pLock;

    // 分发标识 
    std::atomic_flag dispatchTask;

    GlobalData() :dispatchTask(ATOMIC_FLAG_INIT) {}

    CorePoolPtr getPool(const std::string& id);
};

struct RabbitThreadData {
    // 请求任务的数量
    uint64_t reqTaskCnt = 0;
    // 回复任务的数量 
    uint64_t rspTaskCnt = 0;
    // 请求队列
    std::list<RabbitReqDataPtr> reqQueue;
    // 回复队列
    std::mutex rspLock;
    std::list<RabbitRspDataPtr> rspQueue;
    uint32_t lastRunTime = 0;
    // 上一次统计输出时间
    time_t lastPrintTime = 0;
    // 初始化
    bool init = false;
};

///////////////////////////////////////////////////////////////////

bool checkError(amqp_rpc_reply_t x, char const *context);

bool checkError(int x, char const* context);

bool checkConn(amqp_rpc_reply_t x);

bool checkChannel(amqp_rpc_reply_t x);

#define rabbitLog(format, ...) log("[async_rabbitmq] " format, __VA_ARGS__)


}
}