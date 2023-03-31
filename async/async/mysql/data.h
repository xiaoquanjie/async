/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include "async/async/mysql/async_mysql.h"
#include "async/async/mysql/mysql_reply_parser.h"
#include "async/log.h"
#include <mysql/mysql.h>
#include <functional>
#include <list>
#include <atomic>
#include <mutex>
#include <vector>
#include <unordered_map>

namespace async {
namespace mysql {

// mysql连接状态
enum {
    enum_connecting_state,
    enum_connected_state,
    enum_querying_state,
    enum_storing_state,
    // enum_stored_state,
    // enum_error_state,
    // enum_disconnected_state,
};

// mysql地址结构
struct MysqlAddr {
    MysqlAddr (const std::string& id);

    void Parse(const std::string& id);

    std::string id;
    std::string host;
    uint32_t    port = 0;
    std::string db;
    std::string user;
    std::string pwd;
};

typedef std::shared_ptr<MysqlAddr> MysqlAddrPtr;

// 请求
struct MysqlReqData {
    async_mysql_cb cb;
    MysqlAddrPtr addr;
    std::string cmd;
    time_t reqTime = 0;
    bool isQuery = false;
    void* tData = 0;
};

typedef std::shared_ptr<MysqlReqData> MysqlReqDataPtr;

// 回复
struct  MysqlRspData {
    MysqlReqDataPtr     req;
    MysqlReplyParserPtr parser;
};

typedef std::shared_ptr<MysqlRspData> MysqlRspDataPtr;

struct MysqlConn {
    MYSQL *c = 0;
    uint32_t state = enum_connecting_state;
    ~MysqlConn();
};

typedef std::shared_ptr<MysqlConn> MysqlConnPtr;

// 核心
struct MysqlCore {
    MysqlConnPtr conn;
    MysqlAddrPtr addr;
    std::atomic<uint32_t> task;
    bool running = false;
    clock_t lastRunTime = 0;
    MysqlReqDataPtr curReq;
    std::list<MysqlReqDataPtr> waitQueue;
    std::mutex waitLock;
    ~MysqlCore();
};

typedef std::shared_ptr<MysqlCore> MysqlCorePtr;

// 连接池
struct CorePool {
    uint32_t polling = 0;
    std::vector<MysqlCorePtr> valid;
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

struct MysqlThreadData {
    // 请求任务的数量
    uint64_t reqTaskCnt = 0;
    // 回复任务的数量 
    uint64_t rspTaskCnt = 0;
    // 请求队列
    std::unordered_map<std::string, std::list<MysqlReqDataPtr>> reqQueue;
    // 回复队列
    std::mutex rspLock;
    std::list<MysqlRspDataPtr> rspQueue;
    uint32_t lastRunTime = 0;
    // 上一次统计输出时间
    time_t lastPrintTime = 0;
    // 初始化
    bool init = false;
};

///////////////////////////////////////////////////////////////////////////////

#define mysqlLog(format, ...) log("[async_mysql] " format, __VA_ARGS__)


}
}