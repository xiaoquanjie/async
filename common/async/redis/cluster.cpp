/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
// desc: 使用的是同步api，因为异步api无法区分不同连接的redisAsyncContext
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_REDIS

#include "common/async/redis/data.h"
#include <hiredis/hiredis.h>

namespace async {
namespace redis {

extern uint32_t gMaxConnection;

void onThreadResult(void* c, RedisCore* core, redisReply* reply, void* uData);

// 使用集群接口
#ifdef USE_ASYNC_CLUSTER_REDIS

#include <hiredis_cluster/hircluster.h>
#include <hiredis_cluster/adapters/libevent.h>

// 释放
void freeClusterCore(RedisCore* c) {
    if (!c->ctxt) {
        return;
    }

    redisClusterFree((redisClusterContext*)c->ctxt);
}

// 响应连接
void onThreadClusterConnect(const redisAsyncContext* c, int status) {

}

// 响应断线
void onThreadClusterDisconnect(const redisAsyncContext *c, int status) {

}

// 响应授权
void onThreadClusterAuth(redisClusterAsyncContext *c, void* r, void* uData) {

}

// 响应结果 
void onThreadClusterResult(redisClusterAsyncContext *c, void* r, void* uData) {
    
}

void onThreadClusterSpecial(redisClusterAsyncContext* c, void* r, void* uData) {}

// 创建core
RedisCorePtr threadClusterCreateCore(RedisThreadData* tData, RedisAddrPtr addr) {
    if (!addr->cluster) {
        redisLog("[error] failed to call redisAsyncConnect in cluster:%s", addr->id.c_str());
        return nullptr;
    }

    // 取空闲队列
    auto& corePool = tData->corePool[addr->id];
    if (!corePool.valid.empty()) {
        auto core = corePool.valid.front();
        core->curReq = nullptr;
        corePool.valid.pop_front();
        corePool.useing[core->ctxt] = core;
        return core;
    }

    // 正在连接中
    if (corePool.conning.size()) {
        return nullptr;
    }

    // 因为是同步io，所以多个连接没有意义
    uint32_t conns = corePool.useing.size() + corePool.conning.size();
    if (conns >= 1) {
        return nullptr;
    }

    time_t curTime = 0;
    time(&curTime);

    // 不允许短时间内快速创建多个相同断联连接
    if (corePool.lastDisconnect == curTime) {
        return nullptr;
    }

    corePool.lastCreate = curTime;

    redisClusterContext *cc = 0;

    do {
        // 创建新连接
        cc = redisClusterContextInit();
        if (!cc) {
            redisLog("[error] failed to call redisClusterContextInit: %s", addr->host.c_str());
            break;
        }

        RedisCorePtr core = std::make_shared<RedisCore>();
        core->addr = addr;
        redisClusterSetOptionAddNodes(cc, core->addr->host.c_str());

        // 1.5s
        struct timeval timeout = {1, 500000}; 
        redisClusterSetOptionConnectTimeout(cc, timeout);
        redisClusterSetOptionRouteUseSlots(cc);

        if (!core->addr->pwd.empty()) {
            redisClusterSetOptionPassword(cc, core->addr->pwd.c_str());
        }

        redisClusterConnect2(cc);
        if (cc->err) {
            redisLog("[error] failed to call redisClusterConnect2: %s", cc->errstr);
            break;
        }

        redisLog("redisClusterContext:%p|%s connection is successful", cc, core->addr->id.c_str());
        
        if (!core->addr->pwd.empty()) {
            redisLog("redisClusterContext:%p|%s auth successfully", cc, core->addr->id.c_str());
        }

        // 一切就绪
        core->state = enum_selected_state;
        core->ctxt = cc;
        return core;
    } while (false);

    if (cc) {
        redisClusterFree(cc);
    }

    return nullptr;
}

// 执行命令
void threadClusterOp(RedisCorePtr core, void* uData, const BaseRedisCmd& cmd, uint32_t opType) {
    redisClusterContext *cc = (redisClusterContext *)core->ctxt;

    size_t cmdSize = cmd.cmd.size();
    std::vector<const char *> argv(cmdSize);
    std::vector<size_t> argc(cmdSize);
    convertCmd(cmd, argv, argc);

    redisClusterAppendCommandArgv(cc, cmdSize, &(argv[0]), &(argc[0]));

    // 获取结果
    redisReply *reply = 0;
    redisClusterGetReply(cc, (void **)&reply); 

    onThreadResult(cc, core.get(), reply, uData);
}

#else

void freeClusterCore(RedisCore* c) {}

// 响应连接，空实现
void onThreadClusterConnect(const redisAsyncContext* c, int status) {}

// 响应断线
void onThreadClusterDisconnect(const redisAsyncContext *c, int status) {}

// 响应授权
void onThreadClusterAuth(void *c, void* r, void* uData) {}

// 响应结果 
void onThreadClusterResult(void *c, void* r, void* uData) {
    
}

// 创建core
RedisCorePtr threadClusterCreateCore(RedisThreadData* tData, RedisAddrPtr addr) {
    redisLog("[error] not support cluster%s", "");
    return nullptr;
}

void threadClusterOp(RedisCorePtr core, void* uData, const BaseRedisCmd& cmd, uint32_t opType) {}

#endif

} // for redis
} // for async
#endif