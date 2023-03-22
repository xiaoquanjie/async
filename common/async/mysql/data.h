/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include "common/async/mysql/async_mysql.h"
#include "common/async/mysql/mysql_reply_parser.h"
#include "common/log.h"
#include <mysql/mysql.h>
#include <functional>
#include <list>
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
    enum_error_state,
    enum_disconnected_state,
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
};

typedef std::shared_ptr<MysqlReqData> MysqlReqDataPtr;

// 回复
struct  MysqlRspData {
    MysqlReqDataPtr     req;
    MysqlReplyParserPtr parser;
};

typedef std::shared_ptr<MysqlRspData> MysqlRspDataPtr;

// 核心
struct MysqlCore {
    MYSQL *c = 0;
    MysqlAddrPtr addr;
    uint32_t state = enum_connecting_state;
    MysqlReqDataPtr curReq;

    ~MysqlCore();
};

typedef std::shared_ptr<MysqlCore> MysqlCorePtr;

// 连接池
struct CorePool {
    time_t lastCreate = 0;              // 上一次创建core的时间
    time_t lastDisconnect = 0;          // 上一次断联时间
    std::list<MysqlCorePtr> valid;      // 可用的
    std::list<MysqlCorePtr> conning;    // 正在连接的队列
    std::list<MysqlCorePtr> useing;     // 正在使用中的, using是关键字
};

struct MysqlThreadData {
    // 请求任务的数量
    uint64_t reqTaskCnt = 0;
    // 回复任务的数量 
    uint64_t rspTaskCnt = 0;
    // 请求队列
    std::list<MysqlReqDataPtr> reqQueue;
    std::list<MysqlReqDataPtr> prepareQueue;
    // 回复队列
    std::list<MysqlRspDataPtr> rspQueue;
    // 连接池
    std::unordered_map<std::string, CorePool> corePool;
    // 标记
    bool running = false;
    uint32_t lastRunTime = 0;
    // 上一次统计输出时间
    time_t lastPrintTime = 0;
    // 初始化
    bool isInit = false;
};

///////////////////////////////////////////////////////////////////////////////

#define mysqlLog(format, ...) log("[async_mysql] " format, __VA_ARGS__)


}
}