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

    int Handle(const char* packet, uint32_t packet_size);

protected:
    virtual void InCoroutine() = 0;

    virtual bool ParsePacket(const char* packet, uint32_t packet_size) = 0;
protected:
    uint32_t m_req_cmd_id;
    uint32_t m_rsp_cmd_id;
    uint32_t m_trans_id;
    const char* m_packet;
    uint32_t m_packet_size;
};

/////////////////////////////////////////////////////////////////////

// 单向包事务
template<typename RequestType>
class OneWayTransaction : public BaseTransaction {
public:
    int ReturnValue() {
        return m_return_value;
    }

    RequestType& Request() {
        return m_request;
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
    RequestType m_request;   // 请求数据结构
};

/////////////////////////////////////////////////////////////////////

// 双向包事务
template<typename RequestType, typename RespondType>
class TwoWayTransaction : public BaseTransaction { 
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

//////////////////////////////////////////////////////////////////////////////////

// 空回复
struct NullRespond {};

// 利用模板特化重载Transaction类型---三个参数
template<typename RequestType, typename RespondType = NullRespond>
class Transaction 
	: public TwoWayTransaction<RequestType, RespondType> {
};

// 利用模板特化重载Transaction类型---一个参数
template<typename RequestType>
class Transaction<RequestType, NullRespond> 
	: public OneWayTransaction<RequestType> {
public:
};