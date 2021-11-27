/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include <stdint.h>

void _transaction_coroutine_enter_(void*);

class BaseTransaction {
public:
    friend void _transaction_coroutine_enter_(void*);

    void Construct();

    BaseTransaction();

    virtual ~BaseTransaction() {}

    void SetReqCmdId(uint32_t cmd);

    uint32_t ReqCmdId();

    void SetRspCmdId(uint32_t cmd);

    uint32_t RspCmdId();

    void SetTransId(uint32_t trans_id);

    uint32_t TransId();

    int Handle(const char* packet, uint32_t packet_size, void* ext);

protected:
    virtual void InCoroutine() = 0;

    virtual bool ParsePacket(const char* /*packet*/, uint32_t /*packet_size*/) = 0;
protected:
    uint32_t m_req_cmd_id;
    uint32_t m_rsp_cmd_id;
    uint32_t m_trans_id;
    const char* m_packet;
    uint32_t m_packet_size;
    void*    m_ext;
};

/////////////////////////////////////////////////////////////////////

// 空回复
struct NullRespond {};

template<typename RequestType, typename RespondType=NullRespond>
class Transaction : public BaseTransaction {
public:
    int ReturnValue() {
        return m_return_value;
    }

    RequestType& Request() {
        return m_request;
    }

    RespondType& Respond() {
        return m_respond;
    }

    void InCoroutine() override {
        if (!m_packet) {
            return;
        }

        // 解包
        if (!ParsePacket(m_packet, m_packet_size)) {
            return;
        }

        BeforeOnRequest();
        m_return_value = OnRequest();
        AfterOnRequest();
    }

    // 在OnRequest之后调用
    virtual void BeforeOnRequest() {}

    virtual int OnRequest() = 0;

    virtual void AfterOnRequest() {}

protected:
    int m_return_value;      // 返回值
    RequestType m_request;   // 请求的数据结构
    RespondType m_respond;   // 回复的数据结构
};
