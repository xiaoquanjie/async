/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_CURL

#include "common/async/curl/async_curl.h"

#if CUR_CURL_VERSION == 2

#include "common/log.h"
#include <queue>
#include <vector>
#include <string.h>
#include <curl/curl.h>
#include <mutex>
#include <memory>
#include <unordered_map>

namespace async {
namespace curl {

enum HTTP_METHOD { 
    METHOD_GET = 0,
    METHOD_POST,
    METHOD_PUT,
};

// 请求数据
struct CurlReqData {
    async_curl_cb cb;
    CURL *curl = 0;
    struct curl_slist* header = 0;
    std::string rspBody;
    std::string postBody;

    ~CurlReqData() {
        if (header) {
            curl_slist_free_all(header);
        }
        if (curl) {
            curl_easy_cleanup(curl);
        }
    }
};

typedef std::shared_ptr<CurlReqData> CurlReqDataPtr;

struct CurlRspData {
    CURL *curl = 0;
    // curl操作的错误码
    int32_t curlCode = 0;
    // http协议的错误码
    int32_t rspCode = 0;
};

typedef std::shared_ptr<CurlRspData> CurlRspDataPtr;

// 运行时数据
struct CurlThreadData {
    // 请求任务的数量
    uint64_t reqTaskCnt = 0;
    // 回复任务的数量 
    uint64_t rspTaskCnt = 0;
    // 回复队列
    std::queue<CurlRspDataPtr> rspQueue;
    // 多curl句柄
    CURLM* curlm = nullptr;
    std::unordered_map<CURL*, CurlReqDataPtr> curlMap;
    // 标记
    bool running = false;
    // 上一次统计输入时间
    time_t lastPrintTime = 0;

    ~CurlThreadData() {
        if (curlm) {
            curl_multi_cleanup(curlm);
        }
    }
};

// io线程的函数
std::function<void(std::function<void()>)> gIoThr = nullptr;
// 全局初始化
bool gGlobalIsInit = false;
// 最大连接数
uint32_t gMaxConnection = 20;
thread_local CurlThreadData gThrData;

CurlThreadData* threadData() {
    return &gThrData;
}

#define curlLog(format, ...) log("[async_curl] " format, __VA_ARGS__)

void curlGlobalInit() {
    if (gGlobalIsInit) {
        return;
    }

    auto code = curl_global_init(CURL_GLOBAL_ALL);
    if (code != CURLE_OK) {
        curlLog("[error] failed to call curl_global_init: %d", code);
        return;
    }

    gGlobalIsInit = true;
}

void curlGlobalCleanup() {
    if (gGlobalIsInit) {
        curl_global_cleanup();
    }
}

void curlmInit(CurlThreadData* tData) {
    if (tData->curlm) {
        return;
    }

    tData->curlm = curl_multi_init();
    if (!tData->curlm) {
        curlLog("[error] failed to call curl_multi_init%s", "");
        return;
    }

    // 设置连接数量
    curl_multi_setopt(tData->curlm, CURLMOPT_MAX_HOST_CONNECTIONS, gMaxConnection);
}

void threadProcess(CurlThreadData* tData) {
    int stillRunning = 0;
    do {
        curl_multi_wait(tData->curlm, NULL, 0, 0, NULL);
        auto code = curl_multi_perform(tData->curlm, &stillRunning);
        if (code != CURLM_OK) {
            curlLog("[error] failed to call curl_multi_perform: %d", code);
            break;
        }
    } while (stillRunning);

    // 读取数据
    CURLMsg* curlMsg = 0;
    int msgLeft = 0;

    while ((curlMsg = curl_multi_info_read(tData->curlm, &msgLeft))) {
        if (CURLMSG_DONE != curlMsg->msg) {
            continue;
        }

        CurlRspDataPtr rsp = std::make_shared<CurlRspData>();
        rsp->curl = (CURL*)curlMsg->easy_handle;

        // rspCode为0，意味着未没有服务器响应
        rsp->curlCode =  curl_easy_getinfo(rsp->curl, CURLINFO_RESPONSE_CODE, &rsp->rspCode);
        if (CURLE_OK != rsp->curlCode) {
            curlLog("[error] failed to call curl_easy_getinfo: %d", rsp->curlCode);
        }

        tData->rspQueue.push(rsp);
        curl_multi_remove_handle(tData->curlm, rsp->curl);
    }

    tData->running = false;
}

size_t onThreadCurlWriter(void *buffer, size_t size, size_t count, void* param) {
    std::string* s = static_cast<std::string*>(param);
    if (nullptr == s) {
        return 0;
    }

    s->append(reinterpret_cast<char*>(buffer), size * count);
    return size * count;
}

void localRequest(int method, 
                  const std::string& url,
                  const std::map<std::string, std::string> &headers,
                  const std::string &content,
                  async_curl_cb cb) {
    curlGlobalInit();
    
    CurlThreadData* tData = threadData();
    curlmInit(tData);

    CurlReqDataPtr req = std::make_shared<CurlReqData>();
    req->cb = cb;
    req->postBody = content;

    req->curl = curl_easy_init();
    if (!req->curl) {
        curlLog("[error] failed to call curl_easy_init%s", "");
        return;
    }

    curl_easy_setopt(req->curl, CURLOPT_VERBOSE, 0);
    curl_easy_setopt(req->curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(req->curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(req->curl, CURLOPT_TIMEOUT, 15);
    curl_easy_setopt(req->curl, CURLOPT_CONNECTTIMEOUT, 15);
    curl_easy_setopt(req->curl, CURLOPT_FOLLOWLOCATION, true);
    curl_easy_setopt(req->curl, CURLOPT_SSL_VERIFYHOST, false);
    curl_easy_setopt(req->curl, CURLOPT_SSL_VERIFYPEER, false);
    curl_easy_setopt(req->curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(req->curl, CURLOPT_TCP_KEEPIDLE, 120L);
    curl_easy_setopt(req->curl, CURLOPT_TCP_KEEPINTVL, 120L);
    
    curl_easy_setopt(req->curl, CURLOPT_WRITEDATA, &req->rspBody);
    curl_easy_setopt(req->curl, CURLOPT_WRITEFUNCTION, onThreadCurlWriter);

    if (method == METHOD_POST) {
        curl_easy_setopt(req->curl, CURLOPT_POST, 1);
        curl_easy_setopt(req->curl, CURLOPT_POSTFIELDSIZE_LARGE, req->postBody.size());
        curl_easy_setopt(req->curl, CURLOPT_POSTFIELDS, req->postBody.c_str());
    }
    else if (method == METHOD_PUT) {
        curl_easy_setopt(req->curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(req->curl, CURLOPT_POSTFIELDS, req->postBody.c_str());
    }

    // set header
    struct curl_slist* curl_headers = 0;
    curl_headers = curl_slist_append(curl_headers, "connection: keep-alive");
    for (const auto& header : headers) {
        curl_headers = curl_slist_append(curl_headers, (header.first + ": " + header.second).c_str());
    }
    if (curl_headers) {
        curl_easy_setopt(req->curl, CURLOPT_HTTPHEADER, curl_headers);
    }

    req->header = curl_headers;
    
    // curl_multi_add_handle是线程安全的
    auto code = curl_multi_add_handle(tData->curlm, req->curl);
    if (code != CURLM_OK) {
        curlLog("[error] failed to call curl_multi_add_handle: %d", code);
        return;
    }

    tData->reqTaskCnt++;
    tData->curlMap[req->curl] = req;
}

// 取结果 
void localRespond(CurlThreadData* tData) {
    if (tData->rspQueue.empty()) {
        return;
    }

    // 当帧一次性所全部结果取出来
    std::queue<CurlRspDataPtr> que;
    // 交换队列
    que.swap(tData->rspQueue);

    // 回调结果
    while (!que.empty()) {
        tData->rspTaskCnt++;
        CurlRspDataPtr rsp = que.front();
        que.pop();

        auto iter = tData->curlMap.find(rsp->curl);
        if (iter != tData->curlMap.end()) {
            iter->second->cb(rsp->curlCode, rsp->rspCode, iter->second->rspBody);
            tData->curlMap.erase(iter);
        }
    }
}

void localProcess(CurlThreadData* tData) {
    if (tData->curlMap.empty()) {
        return;
    }

    tData->running = true;
    std::function<void()> f = std::bind(threadProcess, tData);
    if (gIoThr) {
        gIoThr(f);
    } else {
        f();
    }
}

void localStatistics(int32_t curTime, CurlThreadData* tData) {
    if (curTime - tData->lastPrintTime < 120) {
        return;
    }

    tData->lastPrintTime = curTime;
    curlLog("[statistics] curTask: %d, reqTask: %d, rspTask: %d",
        tData->reqTaskCnt - tData->rspTaskCnt,
        tData->reqTaskCnt,
        tData->rspTaskCnt);
}

//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////// public //////////////////////////////

void get(const std::string &url, async_curl_cb cb) {
    std::map<std::string, std::string> headers;
    get(url, headers, cb);
}

void get(const std::string &url, const std::map<std::string, std::string>& headers, async_curl_cb cb) {
    localRequest(METHOD_GET, url, headers, "", cb);
}

void post(const std::string& url, const std::string& content, async_curl_cb cb) {
    std::map<std::string, std::string> headers;
    post(url, content, headers, cb);
}

void post(const std::string& url, const std::string& content, const std::map<std::string, std::string>& headers, async_curl_cb cb) {
    localRequest(METHOD_POST, url, headers, content, cb);
}

bool loop(uint32_t curTime) {
    if (!gGlobalIsInit) {
        return false;
    }

    CurlThreadData* tData = threadData();
    if (!tData->curlm) {
        return false;
    }

    if (!tData->running) {
        localRespond(tData);
        localProcess(tData);
    }

    localStatistics(curTime, tData);

    bool hasTask = !tData->curlMap.empty();
    return hasTask;
}

void setThreadFunc(std::function<void(std::function<void()>)> f) {
    gIoThr = f;
}


void setMaxConnection(uint32_t c) {
    gMaxConnection= c;
}

} // curl
} // async

#endif
#endif