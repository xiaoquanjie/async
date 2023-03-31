/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_TRANS

#include "async/transaction/base_transaction.h"
#include "async/coroutine/coroutine.hpp"
#include "async/transaction/transaction_mgr.h"

void BaseTransaction::Construct() {
    m_req_cmd_id = 0;
    m_rsp_cmd_id = 0;
    m_trans_id = 0;
    m_packet = 0;
    m_packet_size = 0;
    m_ext = 0;
}

BaseTransaction::BaseTransaction() {
    Construct();
}

void BaseTransaction::SetReqCmdId(uint32_t cmd) {
    m_req_cmd_id = cmd;
}

uint32_t BaseTransaction::ReqCmdId() {
    return m_req_cmd_id;
}

void BaseTransaction::SetRspCmdId(uint32_t cmd) {
    m_rsp_cmd_id = cmd;
}

uint32_t BaseTransaction::RspCmdId() {
    return m_rsp_cmd_id;
}

void BaseTransaction::SetTransId(uint32_t trans_id) {
    m_trans_id = trans_id;
}

uint32_t BaseTransaction::TransId() {
    return m_trans_id;
}

int BaseTransaction::Handle(const char* packet, uint32_t packet_size, void* ext) {
    // 启动协程
    m_packet = packet;
    m_packet_size = packet_size;
    m_ext = ext;
    CoroutineTask::doTask(_transaction_coroutine_enter_, (void*)this);
    return 0;
}

void _transaction_coroutine_enter_(void* p) {
    BaseTransaction* trans = (BaseTransaction*)p;
    // 运行
    trans->InCoroutine();
    // 协程退出
    trans_mgr::clearTransCxt();
    trans_mgr::recycleTrans(trans);
}

#endif