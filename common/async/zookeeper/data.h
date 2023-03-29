/*----------------------------------------------------------------
// Copyright 2023
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
#include <zookeeper/zookeeper.h>
#include <atomic>
#include <unordered_map>
#include "common/async/zookeeper/async_zookeeper.h"
#include "common/log.h"

namespace async {
namespace zookeeper {

struct ZookCore;

// 地址结构: host[ip1:port1,ip2:port2]|scheme[user:pwd]
struct ZookAddr {
    std::string id;
    std::string host;
    std::string scheme;

    ZookAddr(const std::string& id);

    void parse(const std::string& id);
};

typedef std::shared_ptr<ZookAddr> ZookAddrPtr;

// 请求
struct ZookReqData {
    async_zookeeper_cb cb;
    std::shared_ptr<BaseZookCmd> cmd;
    ZookAddrPtr addr;
    time_t reqTime = 0;
    void* tData = 0;
};

typedef std::shared_ptr<ZookReqData> ZookReqDataPtr;

// 回复
struct ZookRspData {
    ZookParserPtr   parser;
    ZookReqDataPtr  req;
};

typedef std::shared_ptr<ZookRspData> ZookRspDataPtr;

struct Watcher {
    bool dispatch = false;
    bool reg = false;
    bool child = false;
    int32_t once = -1;
    std::string path;
    async_zookeeper_cb cb;
    void* tData = 0;
};

typedef std::shared_ptr<Watcher> WatcherPtr;

struct ZookConn {
    zhandle_t* zh = 0;
    bool connected = false;         // 是否连接上
    int  auth = 0;                  // 0授权未操作，-1授权失败 1授权成功
    ~ZookConn();
};

typedef std::shared_ptr<ZookConn> ZookConnPtr;

struct ZookCore {
    ZookConnPtr conn;
    ZookAddrPtr addr;
    std::atomic<uint32_t> task;
    bool running = false;
    clock_t lastRunTime = 0;
    std::list<ZookReqDataPtr> waitQueue;
    std::list<WatcherPtr> watchers;
    std::mutex waitLock;
    ~ZookCore();
};

typedef std::shared_ptr<ZookCore> ZookCorePtr;

struct CorePool {
    uint32_t polling = 0;
    // 有用的连接
    std::vector<ZookCorePtr> valid;   
    // 监听者
    std::unordered_map<std::string, WatcherPtr> watchers;
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

struct ZookThreadData {
    // 请求任务的数量
    uint64_t reqTaskCnt = 0;
    // 回复任务的数量 
    uint64_t rspTaskCnt = 0;
    // 请求队列
    std::unordered_map<std::string, std::list<ZookReqDataPtr>> reqQueue;
    // 回复队列
    std::mutex rspLock;
    std::list<ZookRspDataPtr> rspQueue;
    uint32_t lastRunTime = 0;
    // 上一次统计输出时间
    time_t lastPrintTime = 0;
    // 初始化
    bool init = false;
};

struct InvokeData {
    ZookReqDataPtr req;
    ZookCorePtr c;
};

const char* state2String(int state);

const char* type2String(int type);

const char* lastError();

bool checkOk(int rc, const char* ctxt);

/////////////////////////////////////////////////////////////////////////////

#define zookLog(format, ...) log("[async_zookeeper] " format, __VA_ARGS__)

}
}