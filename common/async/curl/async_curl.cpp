/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_CURL

#include <vector>
#include "common/async/curl/data.h"
#include "common/async/curl/callback.h"
#include "common/async/curl/op.h"
#include "common/log.h"
#include <curl/curl.h>

namespace async {
    
uint32_t supposeIothread();
clock_t getMilClock();

namespace curl {

#define IO_THREADS async::supposeIothread()

// io线程的函数
std::function<void(std::function<void()>)> gIoThr = nullptr;
// 挂载任务数
uint32_t gMountTask = 10;

//////////////////////////////////////////////////////////////////////////////////////////

CurlThreadData* runningData() {
    thread_local static CurlThreadData gThrData;
    return &gThrData;
}

GlobalData* globalData() {
    static GlobalData gData;
    return &gData;
}

//////////////////////////////////////////////////////////////////////////////////////////

void curlGlobalInit() {
    auto gData = globalData();
    if (gData->init) {
        return;
    }

    auto code = curl_global_init(CURL_GLOBAL_ALL);
    if (code != CURLE_OK) {
        curlLog("[error] failed to call curl_global_init: %d", code);
        return;
    }

    gData->init = true;
}

void curlGlobalCleanup() {
    auto gData = globalData();
    if (gData->init) {
        curl_global_cleanup();
    }
}

CurlCorePtr localCreateCore() {
    auto core = std::make_shared<CurlCore>();
    core->curlm = curl_multi_init();
    if (!core->curlm) {
        curlLog("[error] failed to call curl_multi_init%s", "");
        return nullptr;
    }

    auto gData = globalData();
    if (gData->maxConnection != 0) {
        // 设置连接数量
        curl_multi_setopt(core->curlm, CURLMOPT_MAX_HOST_CONNECTIONS, gData->maxConnection);
    }
    
    return core;
}

void threadPoll(CurlCorePtr c) {
    int stillRunning = 0;
    do {
        curl_multi_wait(c->curlm, NULL, 0, 0, NULL);
        auto code = curl_multi_perform(c->curlm, &stillRunning);
        if (code != CURLM_OK) {
            curlLog("[error] failed to call curl_multi_perform: %d", code);
            break;
        }
    } while (stillRunning);

    // 读取数据
    CURLMsg* curlMsg = 0;
    int msgLeft = 0;

    while ((curlMsg = curl_multi_info_read(c->curlm, &msgLeft))) {
        if (CURLMSG_DONE != curlMsg->msg) {
            continue;
        }

        auto curl = (CURL*)curlMsg->easy_handle;
        auto iter = c->curlMap.find(curl);
        if (iter != c->curlMap.end()) {
            auto req = iter->second;
            c->curlMap.erase(iter);

            int32_t curlCode = 0;
            int32_t rspCode = 0;
            curlCode = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &rspCode);
            if (CURLE_OK != curlCode) {
                curlLog("[error] failed to call curl_easy_getinfo: %d", curlCode);
            }

            c->task--;
            CurlRspDataPtr rsp = std::make_shared<CurlRspData>();
            rsp->req = req;
            rsp->parser = std::make_shared<CurlParser>("", curlCode, rspCode);
            rsp->parser->swapValue(req->rspBody);
            onPushRsp(rsp, req->tData);
        }

        curl_multi_remove_handle(c->curlm, curl);
        curl_easy_cleanup(curl);
    }
}

void threadProcess(std::list<CurlCorePtr> coreList) {
    for (auto c : coreList) {
        c->waitLock.lock();
        std::list<CurlReqDataPtr> waitQueue;
        c->waitQueue.swap(waitQueue);
        c->waitLock.unlock();

        for (auto iter = waitQueue.begin(); iter != waitQueue.end(); iter++) {
            auto req = *iter;
            opRequest(c, req);
        }

        threadPoll(c);
    }

    // 去掉运行标识
    for (auto c : coreList) {
        c->running = false;
    }    
}

void localRequest(int method, 
                  const std::string& url,
                  const std::unordered_map<std::string, std::string> &headers,
                  const std::string &content,
                  async_curl_cb cb,
                  async_curl_cb2 cb2) {
    curlGlobalInit();
    auto* tData = runningData();
    tData->init = true;
    
    CurlReqDataPtr req = std::make_shared<CurlReqData>();
    req->url = url;
    req->method = method;
    req->cb = cb;
    req->cb2 = cb2;
    req->postBody = content;
    req->headers = headers;
    req->tData = tData;

    tData->reqTaskCnt++;
    tData->reqQueue.push_back(req);
}

// 取结果 
void localRespond(CurlThreadData* tData) {
    if (tData->rspQueue.empty()) {
        return;
    }

    // 交换队列
    std::list<CurlRspDataPtr> rspQueue;
    tData->rspLock.lock();
    rspQueue.swap(tData->rspQueue);
    tData->rspLock.unlock();

    for (auto iter = rspQueue.begin(); iter != rspQueue.end(); iter++) {
        auto rsp = *iter;
        tData->rspTaskCnt++;
        if (rsp->req->cb2) {
            rsp->req->cb2(rsp->parser);
        } 
        else if (rsp->req->cb) {
            rsp->req->cb(rsp->parser->getCurlCode(), rsp->parser->getRspCode(), rsp->parser->getValue());
        }
    }
}

void localProcess(uint32_t curTime, CurlThreadData* tData) {
    if (tData->reqQueue.empty()) {
        return;
    }

    uint32_t ioThreads = IO_THREADS;
    auto gData = globalData();
    gData->pLock.lock();

    do {
        auto& pool = gData->corePool;
        for (auto iterQueue = tData->reqQueue.begin(); iterQueue != tData->reqQueue.end(); ) {
            auto req = *iterQueue;
            // 遍历一个有效的core
            CurlCorePtr core;
            if (ioThreads == pool.valid.size()) {
                //curlLog("lun xun.....%s", "");
                // 均匀分布
                core = pool.valid[pool.polling++ % ioThreads];
            }
            else {
                for (auto c : pool.valid) {
                    if (c->task < gMountTask) {
                        core = c;
                        break;
                    }
                }
            }

            if (core) {
                core->waitLock.lock();
                core->waitQueue.push_back(req);
                core->waitLock.unlock();
                core->task++;
                iterQueue = tData->reqQueue.erase(iterQueue);
                continue;
            }

            // 创建新core
            if (pool.valid.size() < ioThreads) {
                auto c = localCreateCore();
                if (c) {
                    pool.valid.push_back(c);
                }
            } 
            else {
                //curlLog("full.......%s", "");
            }

            iterQueue++;
        }

    } while (false);

    gData->pLock.unlock();
}

void localDispatchTask(CurlThreadData* tData) {
    auto gData = globalData();
    // 抢占到分配标识
    if (gData->dispatchTask.test_and_set()) {
        return;
    }

    // 把所有的core分为IO_THREADS组
    uint32_t count = IO_THREADS;
    std::vector<std::list<CurlCorePtr>> coreGroup(count);
    uint32_t groupIdx = 0;

    // 加锁
    gData->pLock.lock();

    for (auto c : gData->corePool.valid) {
        if (c->running) {
            continue;
        }
        if (c->task == 0) {
            continue;
        }

        //curlLog("running.......%s", "");
        c->running = true;
        c->lastRunTime = 0;
        coreGroup[(groupIdx++) % count].push_back(c);
    }

    // 解锁
    gData->pLock.unlock();

    // 清标记
    gData->dispatchTask.clear();    

    for (size_t idx = 0; idx < coreGroup.size(); idx++) {
        auto& g = coreGroup[idx];
        if (g.empty()) {
            continue;
        }
        
        // for (auto c : g) {
        //     curlLog("distpatch task to group: %d and core_nums: %d, task: %d, curlm: %p", idx, g.size(), c->task.load(), c->curlm);
        // }

        if (gIoThr) {
            std::function<void()> f = std::bind(threadProcess, g);
            gIoThr(f);
        } else {
            threadProcess(g);
        }
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

    auto gData = globalData();
    gData->pLock.lock();
    curlLog("[statistics] valid: %d", gData->corePool.valid.size());
    gData->pLock.unlock();
}

//////////////////////////////////////////////////////////////////////////////////

void get(const std::string &url, async_curl_cb cb, const std::unordered_map<std::string, std::string>& headers) {
    localRequest(METHOD_GET, url, headers, "", cb, nullptr);
}

void get(const std::string &url, async_curl_cb2 cb, const std::unordered_map<std::string, std::string>& headers) {
    localRequest(METHOD_GET, url, headers, "", nullptr, cb);
}

void post(const std::string& url, const std::string& content, async_curl_cb cb, const std::unordered_map<std::string, std::string>& headers) {
    localRequest(METHOD_POST, url, headers, content, cb, nullptr);
}

void post(const std::string& url, const std::string& content, async_curl_cb2 cb, const std::unordered_map<std::string, std::string>& headers) {
    localRequest(METHOD_POST, url, headers, content, nullptr, cb);
}

bool loop(uint32_t curTime) {
    auto tData = runningData();
    if (!tData->init) {
        return false;
    }

    localStatistics(curTime, tData);
    localRespond(tData);
    localProcess(curTime, tData);
    localDispatchTask(tData);

    bool hasTask = tData->reqTaskCnt != tData->rspTaskCnt;
    return hasTask;
}

void setThreadFunc(std::function<void(std::function<void()>)> f) {
    gIoThr = f;
}

void setMaxConnection(uint32_t c) {
    auto gData = globalData();
    gData->maxConnection = c;
}

} // curl
} // async

#endif
