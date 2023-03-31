/*----------------------------------------------------------------
// Copyright 2023
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include <curl/curl.h>
#include <memory>
#include <atomic>
#include <list>
#include <mutex>
#include <vector>
#include <memory>
#include <unordered_map>
#include "async/async/curl/async_curl.h"
#include "async/log.h"

namespace async {
namespace curl {

enum HTTP_METHOD { 
    METHOD_GET = 0,
    METHOD_POST,
    METHOD_PUT,
};

// 请求
struct CurlReqData {
    async_curl_cb cb;
    async_curl_cb2 cb2;
    int32_t method = 0;
    std::string url;
    std::unordered_map<std::string, std::string> headers;
    struct curl_slist* header = 0;
    std::string postBody;
    std::string rspBody;
    void* tData = 0;
    ~CurlReqData();
};

typedef std::shared_ptr<CurlReqData> CurlReqDataPtr;

// 回复
struct CurlRspData {
    CurlParserPtr parser;
    CurlReqDataPtr req;
};

typedef std::shared_ptr<CurlRspData> CurlRspDataPtr;

struct CurlCore {
    CURLM* curlm = 0;
    std::atomic<uint32_t> task;
    bool running = false;
    clock_t lastRunTime = 0;
    std::unordered_map<CURL*, CurlReqDataPtr> curlMap;
    std::list<CurlReqDataPtr> waitQueue;
    std::mutex waitLock;
    ~CurlCore();
};

typedef std::shared_ptr<CurlCore> CurlCorePtr;

struct CorePool {
    uint32_t polling = 0;           // 轮询索引
    std::vector<CurlCorePtr> valid;
};

struct GlobalData {
    // 连接池
    CorePool corePool;
    std::mutex pLock;

    // 分发标识 
    std::atomic_flag dispatchTask;

    // 初始化标识
    bool init = false;

    // 最大连接数量
    uint32_t maxConnection = 0;

    GlobalData() :dispatchTask(ATOMIC_FLAG_INIT) {}
};

struct CurlThreadData {
    // 请求任务的数量
    uint64_t reqTaskCnt = 0;
    // 回复任务的数量 
    uint64_t rspTaskCnt = 0;
    // 请求队列
    std::list<CurlReqDataPtr> reqQueue;
    // 回复队列
    std::mutex rspLock;
    std::list<CurlRspDataPtr> rspQueue;
    uint32_t lastRunTime = 0;
    // 上一次统计输出时间
    time_t lastPrintTime = 0;
    // 初始化
    bool init = false;
};

/////////////////////////////////////////////////////////////////////////

#define curlLog(format, ...) log("[async_curl] " format, __VA_ARGS__)

}
}