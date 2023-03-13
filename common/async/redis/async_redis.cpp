/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_REDIS

#include "common/async/redis/data.h"
#include "common/async/redis/redis_exception.h"
#include <hiredis/adapters/libevent.h>
#include <assert.h>

namespace async {
namespace redis {

// 前置声明
RedisCorePtr threadNonClusterCreateCore(RedisThreadData* tData, RedisAddrPtr addr);
RedisCorePtr threadClusterCreateCore(RedisThreadData* tData, RedisAddrPtr addr);
void threadNonClusterOp(void* c, void* uData, const BaseRedisCmd& cmd, uint32_t opType);
void threadClusterOp(RedisCorePtr core, void* uData, const BaseRedisCmd& cmd, uint32_t opType);

///////////////////////////////////////////////////////////////////////////////////////
// io线程的函数
std::function<void(std::function<void()>)> gIoThr = nullptr;
// 最大连接数，连接数越大表现越好
uint32_t gMaxConnection = 100;
// 超时时间
uint32_t gTimeout = 5;

///////////////////////////////////////////////////////////////////////////////////////

RedisThreadData* runningData() {
    thread_local static RedisThreadData gThrData;
    return &gThrData;
}

void localInit(RedisThreadData* tData) {
    if (!tData->evtBase) {
        tData->evtBase = event_base_new();
    }
    if (!tData->evtBase) {
        redisLog("[error] failed to call event_base_new%s", "");
    }
}

// 响应超时
void onThreadRedisTimeout(RedisThreadData* tData, RedisReqDataPtr req) {
    redisReply* r = (redisReply*)hi_calloc(1, sizeof(*r));
    memset(r, 0, sizeof(*r));
    r->type = REDIS_REPLY_SELF_TIMEOUT;

    // 将结果写入队列
    RedisRspDataPtr rsp = std::make_shared<RedisRspData>();
    rsp->reply = r;
    rsp->req = req;
    tData->rspQueue.push_back(rsp);
}

// 状态查询
void threadRedisState(RedisThreadData* tData) {
    for (auto& item : tData->corePool) {
        auto& pool = item.second;
        for (auto iter = pool.valid.begin(); iter != pool.valid.end(); ) {
            auto& core = (*iter);
            if (core->state == enum_disconnected_state) {
                time(&pool.lastDisconnect);
                iter = pool.valid.erase(iter);
            } else {
                iter++;
            }
        }

        for (auto iter = pool.conning.begin(); iter != pool.conning.end(); ) {
            auto& core = (*iter);
            if (core->state == enum_disconnected_state) {
                time(&pool.lastDisconnect);
                iter = pool.conning.erase(iter);
                continue;
            }  

            if (core->state == enum_selected_state) {
                // 挪入可用队列
                pool.valid.push_back(core);
                iter = pool.conning.erase(iter);
                continue;
            }
            
            // 1: select db 2: auth
            int32_t needStat = 0;

            if (core->state == enum_connected_state) {
                if (core->addr->pwd.empty()) {
                    core->state = enum_auth_state;
                } else {
                    // 授权
                    core->state = enum_authing_state;
                    needStat = 2;
                }
            } else if (core->state == enum_auth_state) {
                if (core->addr->cluster) {
                    core->state = enum_selected_state;
                } else {
                    core->state = enum_selecting_state;
                    needStat = 1;
                }
            } 

            // select db
            if (needStat == 1) {
                if (core->addr->cluster) {
                    redisLog("[error] not allowed select db in cluster%s", "");
                } else {
                    // 构造请求
                    RedisReqDataPtr req = std::make_shared<RedisReqData>();
                    req->cmd = SelectCmd(core->addr->dbIdx);
                    core->curReq = req;
                    threadNonClusterOp(core->ctxt, tData, core->curReq->cmd, enum_op_select);
                }
            } else if (needStat == 2) {
                // 构造请求
                RedisReqDataPtr req = std::make_shared<RedisReqData>();
                req->cmd = AuthCmd(core->addr->pwd);
                core->curReq = req;
    
                if (core->addr->cluster) {
                    threadClusterOp(core, tData, core->curReq->cmd, enum_op_auth);
                } else {
                    threadNonClusterOp(core->ctxt, tData, core->curReq->cmd, enum_op_auth);
                }
            }

            iter++;
        }
    }
}

void threadPrepare(RedisThreadData* tData) {
    time_t curTime = 0;
    time(&curTime);

    for (auto iter = tData->prepareQueue.begin(); iter != tData->prepareQueue.end(); ) {
        RedisReqDataPtr req = (*iter);
        if (curTime - req->reqTime > gTimeout) {
            onThreadRedisTimeout(tData, req);
            iter = tData->prepareQueue.erase(iter);
            continue;
        }

        RedisCorePtr core;
        if (req->addr->cluster) {
            core = threadClusterCreateCore(tData, req->addr);
            if (core) {
                core->curReq = req;
                threadClusterOp(core, tData, core->curReq->cmd, enum_op_result);
                iter = tData->prepareQueue.erase(iter);
            }
        } else {
            core = threadNonClusterCreateCore(tData, req->addr);
            if (core) {
                core->curReq = req;
                threadNonClusterOp(core->ctxt, tData, core->curReq->cmd, enum_op_result);
                iter = tData->prepareQueue.erase(iter);
            }
        }

        if (!core) {
            iter++;
            continue;
        }
    }
}

void threadProcess(RedisThreadData* tData) {
    threadPrepare(tData);
    threadRedisState(tData);
    event_base_loop(tData->evtBase, EVLOOP_NONBLOCK);
    tData->running = false;
}

void localRequest(RedisThreadData* tData) {
    if (!tData->reqQueue.empty()) {
        tData->prepareQueue.insert(tData->prepareQueue.end(), tData->reqQueue.begin(), tData->reqQueue.end());
        tData->reqQueue.clear();
    }
}

void localRespond(RedisThreadData* tData) {
    while (!tData->rspQueue.empty()) {
        tData->rspTaskCnt++;
        RedisRspDataPtr rsp = tData->rspQueue.front();
        tData->rspQueue.pop_front();
        RedisReplyParserPtr parser = std::make_shared<RedisReplyParser>(rsp->reply);
        rsp->req->cb(parser);
    }
}

void localProcess(uint32_t curTime, RedisThreadData* tData) {
    if (tData->reqTaskCnt != tData->rspTaskCnt || curTime - tData->lastRunTime > 1) {
        tData->running = true;
        tData->lastRunTime = curTime;
        std::function<void()> f = std::bind(threadProcess, tData);
        if (gIoThr) {
            gIoThr(f);
        } else {
            f();
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

    for (auto& item : tData->corePool) {
        redisLog("[statistics] id: %s, valid: %d, conning: %d, using: %d", item.first.c_str(), item.second.valid.size(), item.second.conning.size(), item.second.useing.size());
    }
}

///////////////////////////////////////////// public //////////////////////////////

// @uri: [host|port|pwd|idx|cluster]
void execute(std::string uri, const BaseRedisCmd& redis_cmd, async_redis_cb cb) {
    RedisThreadData* tData = runningData();
    localInit(tData);

    RedisReqDataPtr req = std::make_shared<RedisReqData>();
    req->cmd = redis_cmd;
    req->cb = cb;
    req->addr = std::make_shared<RedisAddr>(uri);
    time(&req->reqTime);

    tData->reqQueue.push_back(req);
    tData->reqTaskCnt++;
}

// 同步redis接口, 无连接池
//bool executeSync(std::string uri, const BaseRedisCmd& redis_cmd, async_redis_cb cb);

bool loop(uint32_t curTime) {
    RedisThreadData* tData = runningData();
    if (!tData->evtBase) {
        return false;
    }

    localStatistics(curTime, tData);

    if (!tData->running) {
        localRespond(tData);
        localRequest(tData);
        localProcess(curTime, tData);
    }

    bool hasTask = tData->reqTaskCnt != tData->rspTaskCnt;
    return hasTask;
}

void setThreadFunc(std::function<void(std::function<void()>)> f) {
    gIoThr = f;
}

} // redis
} // async

#endif