/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_TRANS

#include "common/transaction/base_transaction.h"
#include "common/coroutine/coroutine.hpp"

void BaseTransaction::Construct() {
    m_req_cmd_id = 0;
    m_rsp_cmd_id = 0;
    m_trans_id = 0;
    m_packet = 0;
    m_packet_size = 0;
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

uint32_t BaseTransaction::SetTransId(uint32_t trans_id) {
    m_trans_id = trans_id;
}

uint32_t BaseTransaction::TransId() {
    return m_trans_id;
}

int BaseTransaction::Handle(const char* packet, uint32_t packet_size) {
    // 启动协程
    m_packet = packet;
    m_packet_size = packet_size;
    CoroutineTask::doTask(_transaction_coroutine_enter_, (void*)this);
    return 0;
}

void _transaction_coroutine_enter_(void* p) {
    BaseTransaction* trans = (BaseTransaction*)p;
    // 运行
    trans->InCoroutine();
    // 协程退出
}

#endif