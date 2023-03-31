/*----------------------------------------------------------------
// Copyright 2023
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

namespace async {
namespace rabbitmq {

void onPushRsp(RabbitRspDataPtr rsp, void* t);

void onTimeout(RabbitReqDataPtr req);

void opAckError(RabbitReqDataPtr req);

bool opCmd(RabbitCorePtr c, uint32_t chId, RabbitReqDataPtr req);

bool opWatch(RabbitCorePtr c);

}
}