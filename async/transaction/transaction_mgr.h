/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include "async/transaction/base_transaction.h"
#include "async/transaction/base_tick_transaction.h"
#include "async/transaction/transaction_bucket.h"
#include <functional>
#include <memory>
#include <string>

namespace trans_mgr {

// 设置最大并行运行trans
void setMaxTrans(uint32_t max_trans);

// 返回0说明启动trans成功, 返回-1说明目前已达到最大trans限制，可保留消息下次再调用
// @ext 额外参数
int handle(uint32_t req_cmd_id, const char* packet, uint32_t packet_size, void* ext = 0);

int handle(uint32_t id, std::string url, const char* packet, uint32_t packet_size, void* ext);

void tick(uint32_t cur_time);

// 注册事务桶
int regBucket(TransactionBucket* bucket);

// 回收事务
void recycleTrans(BaseTransaction* t);

// 注册tick事务
int regTickTrans(TransactionBucket* bucket);

// 注册http事务
int regHttpTrans(uint32_t id, std::string url, TransactionBucket* bucket);

// 设置事务的上下文环境
void setTransCxt(std::shared_ptr<void> ctx);

std::shared_ptr<void> getTransCxt();

void clearTransCxt();
};

#define SetTransCtx(ctx)   trans_mgr::setTransCxt(ctx)
#define GetTransCtx(type)  std::static_pointer_cast<type>(trans_mgr::getTransCxt())

// 注册相关的宏
#define REAL_REGIST_TRANSACTION(t, req_cmd, rsp_cmd) \
static int ret_##t##__LINE__ = trans_mgr::regBucket(new trans_mgr::TransactionBucketImpl<t>(req_cmd, rsp_cmd));

// 简化版注册事务宏
#define REGIST_TRANSACTION(t) REAL_REGIST_TRANSACTION(t##Transaction, Cmd##t##Req, 0)

// 简化版注册事务宏
#define REGIST_TRANSACTION_2(t) REAL_REGIST_TRANSACTION(t##Transaction, Cmd##t##Req, Cmd##t##Rsp)

// 注册tick事务宏
#define REGIST_TICK_TRANSACTION(t) \
static int ret_##t = trans_mgr::regTickTrans(new trans_mgr::TransactionBucketImpl<t>(0, 0));

// 注册http事务宏
#define REGIST_HTTP_TRANSACTION(id, url, t) \
static int ret_##t_##__LINE__ = trans_mgr::regHttpTrans(id, url, new trans_mgr::TransactionBucketImpl<t>(0, 0));
