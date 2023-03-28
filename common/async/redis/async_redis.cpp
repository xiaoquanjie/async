/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_REDIS

#include "common/async/redis/data.h"
#include "common/async/redis/callback.h"
#include "common/async/redis/redis_exception.h"
#include <hiredis/adapters/libevent.h>
#include <assert.h>

namespace async {

uint32_t supposeIothread();
clock_t getMilClock();

namespace redis {

#define IO_THREADS async::supposeIothread()

// 前置声明
RedisConnPtr nonClusterCreateConn(event_base* base, RedisCorePtr core);
RedisConnPtr clusterCreateConn(RedisCorePtr core);

void nonClusterOp(RedisCorePtr core, RedisReqDataPtr req, uint32_t opType);
void clusterOp(RedisCorePtr core, RedisReqDataPtr req);

///////////////////////////////////////////////////////////////////////////////////////
// io线程的函数
std::function<void(std::function<void()>)> gIoThr = nullptr;
// 超时时间
uint32_t gTimeout = 5;
// 挂载任务数
uint32_t gMountTask = 10;

///////////////////////////////////////////////////////////////////////////////////////

RedisThreadData* runningData() {
    thread_local static RedisThreadData gThrData;
    return &gThrData;
}

GlobalData* globalData() {
    static GlobalData gData;
    return &gData;
}

event_base* localCreateEvtBase() {
    auto base = event_base_new();
    if (!base) {
        redisLog("[error] failed to call event_base_new%s", "");
    }
    return base;
}

/////////////////////////////////////////////////////////////////////////

RedisCorePtr localCreateCore(RedisAddrPtr addr) {
    auto core = std::make_shared<RedisCore>();
    core->addr = addr;
    return core;
}

RedisConnPtr threadCreateConn(event_base* base, RedisCorePtr core) {
    if (core->addr->cluster) {
        return clusterCreateConn(core);
    }
    else {
        return nonClusterCreateConn(base, core);
    }
}

void threadPoll(RedisCorePtr c) {
    if (c->addr->cluster) {
        return;
    }
    if (!c->conn) {
        return;
    }

    if (c->conn->state == enum_connected_state) {
        if (c->addr->pwd.empty()) {
            c->conn->state = enum_auth_state;
        } else {
            // 授权
            c->conn->state = enum_authing_state;
            RedisReqDataPtr req = std::make_shared<RedisReqData>();
            req->cmd = std::make_shared<AuthCmd>(c->addr->pwd);
            nonClusterOp(c, req, enum_op_auth);
        }
    }
    else if (c->conn->state == enum_auth_state) {
        // 授权
        c->conn->state = enum_selecting_state;
        RedisReqDataPtr req = std::make_shared<RedisReqData>();
        req->cmd = std::make_shared<SelectCmd>(c->addr->dbIdx);
        nonClusterOp(c, req, enum_op_select);
    }
}

void threadProcess(CorePoolPtr pool) {
    time_t curTime =0;
    time(&curTime);

    for (auto& cm : pool->coreMap) {
        auto c = cm.second;
        if (c->conn && c->conn->state == enum_disconnected_state) {
            c->conn = nullptr;
        }
        if (!c->conn && curTime - c->lastConnTime >= 2 && c->task != 0) {
            c->lastConnTime = curTime;
            c->conn = threadCreateConn(pool->evtBase, c);
        }

        threadPoll(c);

        // 检查任务是否超时
        if (c->addr->cluster) {
            // 不可长时间占用
            std::list<RedisReqDataPtr> waitQueue;
            c->waitLock.lock();
            int count = 5;
            for (auto iter = c->waitQueue.begin(); iter != c->waitQueue.end();) {
                auto req = *iter;
                if (curTime - req->reqTime >= gTimeout) {
                    onTimeout(req, c, req->tData);
                    iter = c->waitQueue.erase(iter);
                    continue;
                }
                
                count--;
                if (c->conn && c->conn->state == enum_selected_state) {
                    waitQueue.push_back(req);
                    iter = c->waitQueue.erase(iter);
                } 
                else {
                    break;
                }
            }
            c->waitLock.unlock();
            
            for (auto req : waitQueue) {
                clusterOp(c, req);
            }
        }
        else {
            c->waitLock.lock();
            for (auto iter = c->waitQueue.begin(); iter != c->waitQueue.end();) {
                auto req = *iter;
                if (curTime - req->reqTime >= gTimeout) {
                    onTimeout(req, c, req->tData);
                    iter = c->waitQueue.erase(iter);
                    continue;
                }
                if (c->conn && c->conn->state == enum_selected_state) {
                    nonClusterOp(c, req, enum_op_result);
                    iter = c->waitQueue.erase(iter);
                }
                else {
                    break;
                }
            }
            c->waitLock.unlock();
        }
    }

    event_base_loop(pool->evtBase, EVLOOP_NONBLOCK);
    // 清除运行标记
    pool->running = false;
}

/////////////////////////////////////////////////////////////////////////

void localRespond(RedisThreadData* tData) {
    if (tData->rspQueue.empty()) {
        return;
    }

    // 交换队列
    std::list<RedisRspDataPtr> rspQueue;
    tData->rspLock.lock();
    rspQueue.swap(tData->rspQueue);
    tData->rspLock.unlock();

    for (auto iter = rspQueue.begin(); iter != rspQueue.end(); iter++) {
        auto rsp = *iter;
        tData->rspTaskCnt++;
        rsp->req->cb(rsp->parser);
    }
}

void localProcess(uint32_t curTime, RedisThreadData* tData) {
    if (tData->reqQueue.empty()) {
        return;
    }

    uint32_t ioThreads = IO_THREADS;
    auto gData = globalData();
    gData->pLock.lock();
    for (auto iter = tData->reqQueue.begin(); iter != tData->reqQueue.end(); iter++) {
        auto req = *iter;
        // 遍历一个有效的corepool
        CorePoolPtr pool;
        if (ioThreads == gData->corePool.size()) {
            pool = gData->corePool[gData->polling++ % ioThreads];
        }
        else {
            for (auto p : gData->corePool) {
                if (p->getTask() < gMountTask) {
                    pool = p;
                    break;
                }
            }
        }
        
        if (!pool) {
            pool = std::make_shared<CorePool>();
            pool->evtBase = localCreateEvtBase();
            gData->corePool.push_back(pool);
        }

        RedisCorePtr core;
        auto coreIter = pool->coreMap.find(req->addr->id);
        if (coreIter == pool->coreMap.end()) {
            core = localCreateCore(req->addr);
            pool->coreMap.insert(std::make_pair(req->addr->id, core));
        } else {
            core = coreIter->second;
        }

        core->waitLock.lock();
        core->waitQueue.push_back(req);
        core->waitLock.unlock();
        core->task++;
    }
    tData->reqQueue.clear();
    gData->pLock.unlock();
}

void localDispatchTask(RedisThreadData* tData) {
    auto gData = globalData();
    // 抢占到分配标识
    if (gData->dispatchTask.test_and_set()) {
        return;
    }

    std::vector<CorePoolPtr> corePool;
    clock_t nowClock = getMilClock();

    // 加锁
    gData->pLock.lock();
    for (auto& p : gData->corePool) {
        if (p->running) {
            continue;
        }
        if (p->getTask() == 0 && nowClock - p->lastRunTime <= 1000) {
            continue;
        }

        p->running = true;
        p->lastRunTime = nowClock;
        corePool.push_back(p);
    }
    // 解锁
    gData->pLock.unlock();

    // 清标记
    gData->dispatchTask.clear();  

    for (size_t idx = 0; idx < corePool.size(); idx++) {
        auto& p = corePool[idx];
        //redisLog("dispatch task to group: %d, task: %d", idx, p->getTask());
        if (gIoThr) {
            std::function<void()> f = std::bind(threadProcess, p);
            gIoThr(f);
        } else {
            threadProcess(p);
        }
    }
}

void localStatistics(int32_t curTime, RedisThreadData* tData) {
    if (curTime - tData->lastPrintTime < 120) {
        return;
    }

    tData->lastPrintTime = curTime;
    redisLog("[statistics] curTask: %d, reqTask: %d, rspTask: %d",
        tData->reqTaskCnt - tData->rspTaskCnt,
        tData->reqTaskCnt,
        tData->rspTaskCnt);

    auto gData = globalData();
    gData->pLock.lock();
    for (auto& pool : gData->corePool) {
        for (auto& cm : pool->coreMap) {
            redisLog("[statistics] id: %s, evtBase: %p", cm.first.c_str(), pool->evtBase);
        }
    }
    gData->pLock.unlock();
}

///////////////////////////////////////////// public //////////////////////////////

// @uri: [host|port|pwd|idx|cluster]
void execute(std::string uri, const BaseRedisCmd& redis_cmd, async_redis_cb cb) {
    auto cmd = std::make_shared<BaseRedisCmd>();
    *cmd = redis_cmd;
    execute(uri, cmd, cb);
}

// 同步redis接口, 无连接池
void execute(std::string uri, std::shared_ptr<BaseRedisCmd> redis_cmd, async_redis_cb cb) {
    auto tData = runningData();
    tData->init = true;

    auto req = std::make_shared<RedisReqData>();
    req->cb = cb;
    req->cmd = redis_cmd;
    req->addr = std::make_shared<RedisAddr>(uri);
    req->tData = tData;
    time(&req->reqTime);

    tData->reqQueue.push_back(req);
    tData->reqTaskCnt++;
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

} // redis
} // async

#endif