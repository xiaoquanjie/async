/*----------------------------------------------------------------
// Copyright 2023
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_REDIS

#include "common/async/redis/data.h"
#include <hiredis/hiredis.h>
#include <hiredis/alloc.h>

namespace async {
namespace redis {

void onPushRsp(RedisRspDataPtr rsp, void* t) {
    RedisThreadData* tData = (RedisThreadData*)t;
    tData->rspLock.lock();
    tData->rspQueue.push_back(rsp);
    tData->rspLock.unlock();
}

void onTimeout(RedisReqDataPtr req, RedisCorePtr c, void* t) {
    c->task--;

    redisReply* r = (redisReply*)hi_calloc(1, sizeof(*r));
    memset(r, 0, sizeof(*r));
    r->type = REDIS_REPLY_SELF_TIMEOUT;

    // 将结果写入队列
    RedisRspDataPtr rsp = std::make_shared<RedisRspData>();
    rsp->req = req;
    rsp->parser = std::make_shared<RedisReplyParser>(r);
    onPushRsp(rsp, t);
}

}
}

#endif