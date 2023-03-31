/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include "async/async/mongo/async_mongo.h"
#include "async/log.h"
#include <mongoc.h>
#include <list>
#include <mutex>
#include <memory>
#include <atomic>
#include <vector>
#include <assert.h>

namespace async {
    
uint32_t supposeIothread();

namespace mongo {

// mongo地址结构
struct MongoAddr {
    // @uri: [mongourl-db-collection]
    MongoAddr(const std::string& uri);

    void Parse(const std::string &uri);

    std::string id;
    std::string host;
    std::string db;
    std::string collection;
    std::string expire_filed;
    uint32_t expire_time = 0;
};

typedef std::shared_ptr<MongoAddr> MongoAddrPtr;

// 请求
struct MongoReqData {
    MongoAddrPtr addr;
    BaseMongoCmd cmd;
    async_mongo_cb cb;
    time_t reqTime = 0;
    void* tData = 0;
};

typedef std::shared_ptr<MongoReqData> MongoReqDataPtr;

struct MongoRspData {
    MongoReqDataPtr req;
    MongoReplyParserPtr parser;
};

typedef std::shared_ptr<MongoRspData> MongoRspDataPtr;

struct MongoConn {
    mongoc_uri_t* mongoc_uri = 0;
    mongoc_client_t* mongoc_client = 0;
    ~MongoConn();
};

typedef std::shared_ptr<MongoConn> MongoConnPtr;

// mongo连接核心
struct MongoCore {
    MongoConnPtr conn;
    MongoAddrPtr addr;
    std::atomic<uint32_t> task;
    bool running = false;
    clock_t lastRunTime = 0;
    std::list<MongoReqDataPtr> waitQueue;
    std::mutex waitLock;
    ~MongoCore();
};

typedef std::shared_ptr<MongoCore> MongoCorePtr;

struct CorePool {
    uint32_t polling = 0;
    std::vector<MongoCorePtr> valid;
};

typedef std::shared_ptr<CorePool> CorePoolPtr;

struct GlobalData {
    // 连接池
    std::unordered_map<std::string, CorePoolPtr> corePool;
    std::mutex pLock;

    bool init = false;

    // 分发标识 
    std::atomic_flag dispatchTask;

    GlobalData() :dispatchTask(ATOMIC_FLAG_INIT) {}

    CorePoolPtr getPool(const std::string& id);
};

struct MongoThreadData {
    // 请求任务的数量
    uint64_t reqTaskCnt = 0;
    // 回复任务的数量 
    uint64_t rspTaskCnt = 0;
    // 请求队列
    std::unordered_map<std::string, std::list<MongoReqDataPtr>> reqQueue;
    // 回复队列
    std::mutex rspLock;
    std::list<MongoRspDataPtr> rspQueue;
    uint32_t lastRunTime = 0;
    // 上一次统计输出时间
    time_t lastPrintTime = 0;
    // 初始化
    bool init = false;
};

///////////////////////////////////////////////////////////////////////////////

#define mongoLog(format, ...) log("[async_mongo] " format, __VA_ARGS__)


}
}