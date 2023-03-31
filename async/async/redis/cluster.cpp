/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
// desc: 使用的是同步api，因为异步api无法区分不同连接的redisAsyncContext
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_REDIS

#include "async/async/redis/data.h"
#include "async/async/redis/callback.h"
#include <hiredis/hiredis.h>

#ifdef USE_ASYNC_CLUSTER_REDIS
#include <hiredis_cluster/hircluster.h>
#endif

namespace async {
namespace redis {

// 释放
void freeClusterCore(void* c) {
#ifdef USE_ASYNC_CLUSTER_REDIS
    if (c) {
        redisClusterFree((redisClusterContext*)c);
    }
#endif
}

// 创建conn
RedisConnPtr clusterCreateConn(RedisCorePtr core) {
    auto conn = std::make_shared<RedisConn>();
    conn->cluster = true;

#ifdef USE_ASYNC_CLUSTER_REDIS
    redisClusterContext *cc = redisClusterContextInit();
    if (!cc) {
        redisLog("[error] failed to call redisClusterContextInit: %s", core->addr->id.c_str());
        return nullptr;
    }

    conn->ctxt = cc;
    conn->id = core->addr->id;
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
        return nullptr;
    }

    redisLog("redisClusterContext:%p|%s connection is successful", cc, conn->id.c_str());
        
    if (!core->addr->pwd.empty()) {
        redisLog("redisClusterContext:%p|%s auth successfully", cc, conn->id.c_str());
    }

    // 一切就绪
    conn->state = enum_selected_state;
#endif

    return conn;
}

// 执行命令
void clusterOp(RedisCorePtr core, RedisReqDataPtr req) {
#ifdef USE_ASYNC_CLUSTER_REDIS
    redisClusterContext *cc = (redisClusterContext *)core->conn->ctxt;

    size_t cmdSize = req->cmd->cmd.size();
    std::vector<const char *> argv(cmdSize);
    std::vector<size_t> argc(cmdSize);
    convertCmd(*(req->cmd), argv, argc);

    int status = redisClusterAppendCommandArgv(cc, cmdSize, &(argv[0]), &(argc[0]));

    // 获取结果
    redisReply *reply = 0;
    redisClusterGetReply(cc, (void **)&reply); 

    core->task--;
    auto r = copyRedisReply(reply);

    // 将结果写入队列
    RedisRspDataPtr rsp = std::make_shared<RedisRspData>();
    rsp->req = req;
    rsp->parser = std::make_shared<RedisReplyParser>(r);
    onPushRsp(rsp, req->tData);

    if (status != REDIS_OK) {
        redisLog("[error] failed to call redisClusterAppendCommandArgv: %s", cc->errstr);
        core->conn->state = enum_disconnected_state;
    }
    
#endif
}


} // for redis
} // for async
#endif