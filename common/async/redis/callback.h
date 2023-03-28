/*----------------------------------------------------------------
// Copyright 2023
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

namespace async {
namespace redis {

void onPushRsp(RedisRspDataPtr rsp, void* t);

void onTimeout(RedisReqDataPtr req, RedisCorePtr c, void* t);

}
}