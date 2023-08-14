/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
// 
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_REDIS

#include "async/async/redis/data.h"
#include <hiredis/hiredis.h>
#include <hiredis/adapters/libevent.h>
#include "async/async/redis/callback.h"

namespace async {
namespace redis {

extern uint32_t gMaxConnection;

void freeNonClusterCore(void* c) {
    if (!c) {
        return;
    } 
    
    // 关闭回调
    redisAsyncSetDisconnectCallback((redisAsyncContext *)c, 0);
    // 主动关闭连接
    //redisAsyncDisconnect((redisAsyncContext *)c);
    redisAsyncFree((redisAsyncContext *)c);
}

// 响应连接
void onNonClusterConnect(const redisAsyncContext *c, int status) {
    RedisConn* conn = (RedisConn*)c->data;
    if (!conn) {
        redisLog("[error] not recognized redisAsyncContext: %p connection", c);
        return;
    }

    if (status == REDIS_OK) {
        conn->state = enum_connected_state;
        redisLog("redisAsyncContext:%p|%s connection is successful", c, conn->id.c_str());
    } else {
        conn->activeClosed = false;
        conn->state = enum_disconnected_state;
        redisLog("[error] redisAsyncContext:%p|%s connection is failed", c, conn->id.c_str());
    }
}

// 响应断线
void onNonClusterDisconnect(const redisAsyncContext *c, int status) {
    RedisConn* conn = (RedisConn*)c->data;
    if (!conn) {
        redisLog("[error] not recognized redisAsyncContext: %p connection", c);
        return;
    }

    conn->activeClosed = false;
    conn->state = enum_disconnected_state;
    redisLog("redisAsyncContext:%p|%s connection is breaking", c, conn->id.c_str());
}

// 响应授权
void onNonClusterAuth(redisAsyncContext *c, void* r, void* uData) {
    RedisConn* conn = (RedisConn*)(c->data);
    if (!conn) {
        redisLog("[error] not recognized redisAsyncContext: %p connection", c);
        return;
    }

    if (!r) {
        conn->state = enum_disconnected_state;
        redisLog("[error] failed to auth, redisAsyncContext:%p|%s", c, conn->id.c_str());
        return;
    }

    redisReply* reply = (redisReply*)r;
    if (reply->type == REDIS_REPLY_STATUS
        && strcmp(reply->str, "OK") == 0) {
        conn->state = enum_auth_state;
        redisLog("redisAsyncContext:%p|%s auth successfully", c, conn->id.c_str());
    } else {
        conn->state = enum_disconnected_state;
        redisLog("[error] failed to auth, redisAsyncContext:%p|%s|%d", c, conn->id.c_str(), reply->type);
    }
}

// 响应选库
void onNonClusterSelect(redisAsyncContext *c, void* r, void* uData) {
    RedisConn* conn = (RedisConn*)(c->data);
    if (!conn) {
        redisLog("[error] not recognized redisAsyncContext: %p connection", c);
        return;
    }

    if (!r) {
        conn->state = enum_disconnected_state;
        redisLog("[error] failed to select db, redisAsyncContext:%p|%s", c, conn->id.c_str());
        return;
    }

    redisReply* reply = (redisReply*)r;
    if (reply->type == REDIS_REPLY_STATUS
        && strcmp(reply->str, "OK") == 0) {
        conn->state = enum_selected_state;
        redisLog("redisAsyncContext:%p|%s select db successfully", c, conn->id.c_str());
    } else {
        conn->state = enum_disconnected_state;
        redisLog("[error] failed to select db, redisAsyncContext:%p|%s|%d", c, conn->id.c_str(), reply->type);
    }
}

void onNonClusterResult(redisAsyncContext *c, void* r, void* uData) {
    InvokeData* invoke = (InvokeData*)uData;
    RedisReqDataPtr req = invoke->req;
    RedisCorePtr core = invoke->core;
    delete invoke;

    core->task--;
    auto reply = copyRedisReply((redisReply*)r);

    // 将结果写入队列
    RedisRspDataPtr rsp = std::make_shared<RedisRspData>();
    rsp->req = req;
    rsp->parser = std::make_shared<RedisReplyParser>(reply);
    onPushRsp(rsp, req->tData);
}

RedisConnPtr nonClusterCreateConn(event_base* base, RedisCorePtr core) {
    auto conn = std::make_shared<RedisConn>();
    conn->cluster = false;
    redisAsyncContext* c = redisAsyncConnect(core->addr->host.c_str(), core->addr->port);
    if (!c) {
        redisLog("[error] failed to call redisAsyncConnect: %s", core->addr->id.c_str());
        return nullptr;
    } 

    conn->ctxt = c;
    conn->id = core->addr->id;
    c->data = conn.get();

    // set callback
    redisLibeventAttach(c, base);
    redisAsyncSetConnectCallback(c, onNonClusterConnect);
    redisAsyncSetDisconnectCallback(c, onNonClusterDisconnect);    

    return conn;
}

// 执行命令
void nonClusterOp(RedisCorePtr core, RedisReqDataPtr req, uint32_t opType) {
    redisCallbackFn *fn = 0;
    if (opType == enum_op_auth) {
        fn = onNonClusterAuth;
    } else if (opType == enum_op_select) {
        fn = onNonClusterSelect;
    } else if (opType == enum_op_result) {
        fn = onNonClusterResult;
    } else {
        redisLog("[error] failed to call nonClusterOp, error opType: %d", opType);
        return;
    }

    size_t cmdSize = req->cmd->cmd.size();
    std::vector<const char *> argv(cmdSize);
    std::vector<size_t> argc(cmdSize);
    convertCmd(*(req->cmd.get()), argv, argc);

    InvokeData* invoke = 0;
    if (opType == enum_op_result) {
        invoke = new InvokeData();
        invoke->req = req;
        invoke->core = core;
    }    

    redisAsyncCommandArgv((redisAsyncContext*)core->conn->ctxt, 
        fn, 
        invoke,
        cmdSize,
        &(argv[0]),
        &(argc[0])
    );
}


}
}

#endif