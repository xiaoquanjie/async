/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_MONGO

#include "common/async/mongo/data.h"
#include "common/async/mongo/op.h"

namespace async {
namespace mongo {

#define IO_THREADS async::supposeIothread()

// io线程的函数
std::function<void(std::function<void()>)> gIoThr = nullptr;
// 超时时间
uint32_t gTimeout = 10;

///////////////////////////////////////////////////////////////////////

MongoThreadData* runningData() {
    thread_local static MongoThreadData gThrData;
    return &gThrData;
}

GlobalData* globalData() {
    static GlobalData gData;
    return &gData;
}

bool mongoGlobalInit() {
    auto gData = globalData();
    if (gData->init) {
        return true;
    }

    mongoc_init();
    gData->init = true;
    return true;
}

bool mongoGlobalCleanup() {
    mongoc_cleanup();
    return true;
}

///////////////////////////////////////////////////////////////////////

MongoCorePtr localCreateCore(MongoAddrPtr addr) {
    MongoCorePtr core = std::make_shared<MongoCore>();
    core->addr = addr;
    return core;
}

MongoConnPtr threadCreateConn(MongoCorePtr core) {
    auto conn = std::make_shared<MongoConn>();

    // create mongoc uri
    bson_error_t error;
    conn->mongoc_uri = mongoc_uri_new_with_error(core->addr->host.c_str(), &error);
    if (!conn->mongoc_uri) {
        mongoLog("[error] failed to call mongoc_uri_new_with_error:%s", core->addr->host.c_str());
        return nullptr;
    }

    conn->mongoc_client = mongoc_client_new_from_uri(conn->mongoc_uri);
    if (!conn->mongoc_client) {
        mongoLog("[error] failed to call mongoc_client_new_from_uri:%s", core->addr->host.c_str());
        return nullptr;
    }

    mongoc_client_set_appname(conn->mongoc_client, core->addr->host.c_str());
    mongoLog("create connection: %s", core->addr->host.c_str());
    return conn;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void threadProcess(std::list<MongoCorePtr> coreList) {
    for (auto c : coreList) {
        if (!c->conn) {
            c->conn = threadCreateConn(c);
        }

        // 不可长时间占用cpu
        int count = 5;
        c->waitLock.lock();
        for (auto iter = c->waitQueue.begin(); iter != c->waitQueue.end();) {
            if (count == 0) {
                break;
            }
            count--;
            auto req = *iter;
            opCmd(c, req);
            iter = c->waitQueue.erase(iter);
        }
        c->waitLock.unlock();
        c->running = false;
    }
}

void localRespond(MongoThreadData* tData) {
    if (tData->rspQueue.empty()) {
        return;
    }

    // 交换队列
    std::list<MongoRspDataPtr> rspQueue;
    tData->rspLock.lock();
    rspQueue.swap(tData->rspQueue);
    tData->rspLock.unlock();

    for (auto iter = rspQueue.begin(); iter != rspQueue.end(); iter++) {
        auto rsp = *iter;
        tData->rspTaskCnt++;
        rsp->req->cb(rsp->parser);
    }
}

void localProcess(uint32_t curTime, MongoThreadData* tData) {
    if (tData->reqQueue.empty()) {
        return;
    }

    uint32_t ioThreads = IO_THREADS;
    auto gData = globalData();
    gData->pLock.lock();
    for (auto iterQueue = tData->reqQueue.begin(); iterQueue != tData->reqQueue.end(); iterQueue++) {
        auto& id = iterQueue->first;
        auto& que = iterQueue->second;
        auto pool = gData->getPool(id);

        for (auto iterReq = que.begin(); iterReq != que.end(); iterReq++) {
            auto req = *iterReq;
            if (pool->valid.size() < ioThreads) {
                auto c = localCreateCore(req->addr);
                pool->valid.push_back(c);
            }

            auto count = pool->valid.size();
            auto core = pool->valid[pool->polling++ % count];
            core->waitLock.lock();
            core->waitQueue.push_back(req);
            core->waitLock.unlock();
            core->task++;
        }
        que.clear();
    }
    gData->pLock.unlock();
}

void localDispatchTask(MongoThreadData* tData) {
    auto gData = globalData();
    // 抢占到分配标识
    if (gData->dispatchTask.test_and_set()) {
        return;
    }

    // 把所有的core分为IO_THREADS组
    uint32_t count = IO_THREADS;
    std::vector<std::list<MongoCorePtr>> coreGroup(count);
    uint32_t groupIdx = 0;

    // 加锁
    gData->pLock.lock();
    for (auto& poolItem : gData->corePool) {
        auto pool = poolItem.second;
        for (auto c : pool->valid) {
            if (c->running) {
                continue;
            }
            if (c->task == 0) {
                continue;
            }
           
            //mongoLog("running......%s", c->addr->id.c_str());
            c->running = true;
            c->lastRunTime = 0;
            coreGroup[(groupIdx++) % count].push_back(c);
        }
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
        
        // for test
        // for (auto c : g) {
        //      mongoLog("distpatch task to group: %d and core_nums: %d, task: %d, core: %p", idx, g.size(), c->task.load(), c.get());
        // }
        
        if (gIoThr) {
            std::function<void()> f = std::bind(threadProcess, g);
            gIoThr(f);
        } else {
            threadProcess(g);
        }
    } 
}

void localStatistics(uint32_t curTime, MongoThreadData* tData) {
    if (curTime - tData->lastPrintTime < 120) {
        return;
    }

    tData->lastPrintTime = curTime;

    mongoLog("[statistics] cur_task:%d, req_task:%d, rsp_task:%d",
        tData->reqTaskCnt - tData->rspTaskCnt,
        tData->reqTaskCnt,
        tData->rspTaskCnt);

    auto gData = globalData();
    gData->pLock.lock();
    for (auto& pool : gData->corePool) {
        mongoLog("[statistics] id: %s, valid: %d", pool.first.c_str(), pool.second->valid.size());
    }
    gData->pLock.unlock();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void execute(std::string uri, const BaseMongoCmd& cmd, async_mongo_cb cb) {
    mongoGlobalInit();
    auto tData = runningData();
    tData->init = true;

    // 构造一个请求
    MongoReqDataPtr req = std::make_shared<MongoReqData>();
    req->cmd = cmd;
    req->addr = std::make_shared<MongoAddr>(uri);
    req->cb = cb;
    req->tData = tData;
    time(&req->reqTime);

    auto& que = tData->reqQueue[req->addr->id];
    que.push_back(req);
    tData->reqTaskCnt++;
}

bool loop(uint32_t curTime) {
    MongoThreadData* tData = runningData();
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


} // mongo
} // async

#endif