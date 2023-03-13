/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include "common/async/mongo/async_mongo.h"
#include "common/log.h"
#include <mongoc.h>
#include <list>
#include <mutex>
#include <memory>
#include <assert.h>

namespace async {

// 声明
void split(const std::string source, const std::string &separator, std::vector<std::string> &array);

namespace mongo {

// mongo地址结构
struct MongoAddr {
    // @uri: [mongourl-db-collection]
    MongoAddr(const std::string& uri);

    void Parse(const std::string &uri);

    std::string uri;
    std::string addr;
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
};

typedef std::shared_ptr<MongoReqData> MongoReqDataPtr;

struct MongoRspData {
    MongoReqDataPtr req;
    MongoReplyParserPtr parser;
    MongoRspData();
};

typedef std::shared_ptr<MongoRspData> MongoRspDataPtr;

// mongo连接核心
struct MongoCore {
    MongoAddrPtr addr;
    mongoc_uri_t* mongoc_uri = 0;
    mongoc_client_t* mongoc_client = 0;

    MongoCore() {}

    ~MongoCore();
};

typedef std::shared_ptr<MongoCore> MongoCorePtr;

struct CorePool {
    time_t lastCreate = 0;            // 上一次创建core的时间
    time_t lastDisconnect = 0;        // 上一次断联时间
    std::list<MongoCorePtr> valid;    // 可用的
};

struct MongoThreadData {
    std::list<MongoReqDataPtr> reqQueue;
    std::list<MongoRspDataPtr> rspQueue;
    // 连接池
    std::unordered_map<std::string, CorePool> corePool;
    std::mutex lock;
    // 请求任务的数量
    uint64_t reqTaskCnt = 0;
    // 回复任务的数量 
    uint64_t rspTaskCnt = 0;
     // 上一次统计输出时间
    time_t lastPrintTime = 0;
};

///////////////////////////////////////////////////////////////////////////////

#define mongoLog(format, ...) log("[async_mongo] " format, __VA_ARGS__)


}
}