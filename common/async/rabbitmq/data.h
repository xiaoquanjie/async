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
#include <rabbitmq-c/amqp.h>
#include "common/async/rabbitmq/async_rabbitmq.h"
#include "common/log.h"

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
    async_rabbit_get_cb get_cb;
    std::shared_ptr<BaseRabbitCmd> cmd;
    RabbitAddrPtr addr;
    time_t reqTime = 0;
};

typedef std::shared_ptr<RabbitReqData> RabbitReqDataPtr;

// 回复
struct RabbitRspData {
    bool ok = false;
    amqp_rpc_reply_t reply;
    amqp_envelope_t envelope;
    amqp_message_t message;
    async_rabbit_cb cb;
    async_rabbit_get_cb get_cb;
    async_rabbit_watch_cb watch_cb;
};

typedef std::shared_ptr<RabbitRspData> RabbitRspDataPtr;

struct RabbitCore {
    amqp_connection_state_t conn;
    RabbitAddrPtr addr;
    uint32_t chId = 0;
    ~RabbitCore();
};

typedef std::shared_ptr<RabbitCore> RabbitCorePtr;

struct RabbitChannel {
    std::string id;
    uint32_t chId = 0;
    amqp_connection_state_t conn;
    RabbitCorePtr core;
};

typedef std::shared_ptr<RabbitChannel> RabbitChannelPtr;

struct AckInfo {
    std::shared_ptr<AckCmd> cmd;
    async_rabbit_cb cb;
};

struct WatcherInfo {
    void* tData;
    async_rabbit_watch_cb cb;
    std::list<AckInfo> ack_cbs;
    uint32_t chId = 0;
    std::shared_ptr<WatchCmd> cmd;
    bool cancel = false;
    time_t lastCreate = 0; 
};

struct RabbitWatcher {
    time_t lastCreate = 0; 
    uint32_t genChId = 1;
    RabbitCorePtr core;
    std::unordered_map<std::string, WatcherInfo> cbs;   // queue_name_consumer_tag --> cb info
};

struct CorePool {
    //time_t lastCreate = 0;            // 上一次创建core的时间
    //time_t lastDisconnect = 0;        // 上一次断联时间
    std::list<RabbitCorePtr> valid;     // 
};

struct GlobalData {
    // 连接池
    std::unordered_map<std::string, CorePool> corePool;
    std::mutex lock;
    
    // 监听者
    std::unordered_map<std::string, RabbitWatcher> watchPool;
    std::mutex watchLock;
    uint32_t watchIdle = 0;
    clock_t watchIdleTick = 0;
    std::atomic_flag watcherRun;

    GlobalData() : watcherRun(ATOMIC_FLAG_INIT) {}
};

struct RabbitThreadData {
    // 请求任务的数量
    uint64_t reqTaskCnt = 0;
    // 回复任务的数量 
    uint64_t rspTaskCnt = 0;
    // 请求队列
    std::list<RabbitReqDataPtr> reqQueue;
    // 回复队列
    std::mutex lock;
    std::list<RabbitRspDataPtr> rspQueue;
    uint32_t lastRunTime = 0;
    // 上一次统计输出时间
    time_t lastPrintTime = 0;
};

#define rabbitLog(format, ...) log("[async_rabbitmq] " format, __VA_ARGS__)


}
}