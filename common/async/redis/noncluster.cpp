/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
// 
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_REDIS

#include "common/async/redis/data.h"
#include <hiredis/hiredis.h>
#include <hiredis/adapters/libevent.h>

namespace async {
namespace redis {

extern uint32_t gMaxConnection;

void freeNonClusterCore(RedisCore* c) {
    if (!c->ctxt) {
        return;
    }

    // 关闭回调
    redisAsyncSetDisconnectCallback((redisAsyncContext *)c->ctxt, 0);

    // 主动关闭连接
    if (!c->passiveClose) {
        redisAsyncDisconnect((redisAsyncContext *)c->ctxt);
    }

    redisAsyncFree((redisAsyncContext *)c->ctxt);
}

// 响应连接
void onThreadNonClusterConnect(const redisAsyncContext *c, int status) {
    RedisCore* core = (RedisCore*)c->data;
    if (!core) {
        redisLog("[error] not recognized redisAsyncContext: %p connection", c);
        return;
    }

    if (status == REDIS_OK) {
        core->state = enum_connected_state;
        redisLog("redisAsyncContext:%p|%s connection is successful", c, core->addr->id.c_str());
    } else {
        core->state = enum_disconnected_state;
        core->passiveClose = true;
        redisLog("[error] redisAsyncContext:%p|%s connection is failed", c, core->addr->id.c_str());
    }
}

// 响应断线
void onThreadNonClusterDisconnect(const redisAsyncContext *c, int status) {
    RedisCore* core = (RedisCore*)c->data;
    if (!core) {
        redisLog("[error] not recognized redisAsyncContext: %p connection", c);
        return;
    }

    core->state = enum_disconnected_state;
    core->passiveClose = true;
    redisLog("redisAsyncContext:%p|%s is connection is breaking", c, core->addr->id.c_str());
}

// 响应授权
void onThreadNonClusterAuth(redisAsyncContext *c, void* r, void* uData) {
    RedisCore* core = (RedisCore*)(c->data);
    if (!core) {
        redisLog("[error] not recognized redisAsyncContext: %p connection", c);
        return;
    }

    redisReply* reply = (redisReply*)r;
    if (reply->type == REDIS_REPLY_STATUS
        && strcmp(reply->str, "OK") == 0) {
        core->state = enum_auth_state;
        redisLog("redisAsyncContext:%p|%s auth successfully", c, core->addr->id.c_str());
    } else {
        core->state = enum_disconnected_state;
        redisLog("[error] failed to auth, redisAsyncContext:%p|%s|d", c, core->addr->pwd.c_str(), reply->type);
    }

    core->curReq = nullptr;
}

// 响应选库
void onThreadNonClusterSelect(redisAsyncContext *c, void* r, void* uData) {
    RedisCore* core = (RedisCore*)(c->data);
    if (!core) {
        redisLog("[error] not recognized redisAsyncContext: %p connection", c);
        return;
    }

    redisReply* reply = (redisReply*)r;
    if (reply->type == REDIS_REPLY_STATUS
        && strcmp(reply->str, "OK") == 0) {
        core->state = enum_selected_state;
        redisLog("redisAsyncContext:%p|%s select db successfully", c, core->addr->id.c_str());
    } else {
        core->state = enum_disconnected_state;
        redisLog("[error] failed to select db, redisAsyncContext:%p|%s|%d", c, core->addr->id.c_str(), reply->type);
    }

    core->curReq = nullptr;
}

// 响应结果
void onThreadResult(void* c, RedisCore* core, redisReply* reply, void* uData) {
    do {
        RedisThreadData* tData = (RedisThreadData*)uData;
        if (!tData) {
            redisLog("[error] no RedisThreadData%s", "");
            break;
        }

        if (!core) {
            redisLog("[error] RedisCore is null%s", "");
            break;
        }

        if (!core->curReq) {
            redisLog("[error] no current RedisReqData in core%s", "");
            break;
        }

        // 将结果写入队列
        RedisRspDataPtr rsp = std::make_shared<RedisRspData>();
        rsp->reply = reply;
        rsp->req = core->curReq;
        tData->rspQueue.push_back(rsp);

        // reset
        core->curReq = nullptr;

        // 归还core到队列
        auto id = core->addr->id;
        auto& pool = tData->corePool[id];
        // 从使用队列删除
        auto coreIter = pool.useing.find(c);
        if (coreIter != pool.useing.end()) {
            pool.valid.push_back(coreIter->second);
            pool.useing.erase(coreIter);
        }

        return;
    } while (false);

    freeReplyObject(reply);
}

void onThreadNonClusterResult(redisAsyncContext *c, void* r, void* uData) {
    RedisCore* core = (RedisCore*)(c->data);
    auto reply = copyRedisReply((redisReply*)r);
    onThreadResult(c, core, reply, uData);
}

// 创建core
RedisCorePtr threadNonClusterCreateCore(RedisThreadData* tData, RedisAddrPtr addr) {
    if (addr->cluster) {
        redisLog("[error] failed to call redisClusterAsyncConnect in noncluster:%s", addr->id.c_str());
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

    // 超出最大连接数
    uint32_t conns = corePool.useing.size() + corePool.conning.size();
    if (conns >= gMaxConnection) {
        return nullptr;
    }

    time_t curTime = 0;
    time(&curTime);

    // 不允许短时间内快速创建多个相同断联连接
    if (corePool.lastDisconnect == curTime) {
        return nullptr;
    }

    corePool.lastCreate = curTime;

    // 创建新连接
    RedisCorePtr core = std::make_shared<RedisCore>();
    core->addr = addr;

    redisAsyncContext* c = redisAsyncConnect(core->addr->host.c_str(), core->addr->port);
    if (!c) {
        redisLog("[error] failed to call redisAsyncConnect: %s:%d", core->addr->host.c_str(), core->addr->port);
        return nullptr;
    }

    core->ctxt = c;
    c->data = core.get();
    corePool.conning.push_back(core);

    // set callback
    redisLibeventAttach(c, tData->evtBase);
    redisAsyncSetConnectCallback(c, onThreadNonClusterConnect);
    redisAsyncSetDisconnectCallback(c, onThreadNonClusterDisconnect);       

    return nullptr;
}

// 执行命令
void threadNonClusterOp(void* c, void* uData, const BaseRedisCmd& cmd, uint32_t opType) {
    redisCallbackFn *fn = 0;
    if (opType == enum_op_auth) {
        fn = onThreadNonClusterAuth;
    } else if (opType == enum_op_select) {
        fn = onThreadNonClusterSelect;
    } else if (opType == enum_op_result) {
        fn = onThreadNonClusterResult;
    } else {
        redisLog("[error] failed to call threadNonClusterOp, error opType: %d", opType);
        return;
    }

    size_t cmdSize = cmd.cmd.size();
    std::vector<const char *> argv(cmdSize);
    std::vector<size_t> argc(cmdSize);
    convertCmd(cmd, argv, argc);

    redisAsyncCommandArgv((redisAsyncContext*)c, 
        fn, 
        uData,
        cmdSize,
        &(argv[0]),
        &(argc[0])
    );
}

}
}

#endif