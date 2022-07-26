/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include <string>
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

// http的事务
class HttpTransaction : public BaseTransaction {
public:
    const std::string& Url() {
        return m_url;
    }

    void SetUrl(const std::string& url) {
        m_url = url;
    }
    
    void InCoroutine() override {
        if (!m_packet) {
            return;
        }

        // 解包
        if (!ParsePacket(m_packet, m_packet_size)) {
            return;
        }

        if (!OnBefore()) {
            return;
        }

        OnRequest();
        OnAfter();
    } 

    virtual bool OnBefore() { return false; }

    virtual int OnRequest() = 0;

    virtual bool OnAfter() { return false; }

protected:
    std::string m_url;
};

/////////////////////////////////////////////////////////////////////

// 空回复
struct NullRespond {
    bool SerializeToString(std::string*) const {return false;}
};

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

        if (!OnBefore()) {
            return;
        }

        m_return_value = OnRequest();
        OnAfter();
    }

    // 在OnRequest之后调用
    virtual bool OnBefore() { return false; }

    virtual int OnRequest() = 0;

    virtual bool OnAfter() { return false; }

protected:
    int m_return_value;      // 返回值
    RequestType m_request;   // 请求的数据结构
    RespondType m_respond;   // 回复的数据结构
};
