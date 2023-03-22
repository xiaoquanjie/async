/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include "common/transaction/transaction_mgr.h"
#include "serve/serve/base.h"
#include "serve/serve/http.h"
#include <assert.h>

class HttpTransaction : public BaseHttpTransaction {
public:
    using BaseTrans = BaseHttpTransaction;

    struct Request {
        void* r;
        std::string url;
        std::string host;
        std::string squery;
        std::string body;
        std::unordered_map<std::string, std::string> query;
    };

    struct Respond {
        Respond() {
            header["Content-Type"] = "text/plain";
        }
        std::string body;
        std::unordered_map<std::string, std::string> header;
    };

    bool ParsePacket(const char* packet, uint32_t /*packet_size*/) override {
        HttpMsg* msg = const_cast<HttpMsg*>((const HttpMsg*)packet);
        m_request.swap(msg->req);
        m_respond.swap(msg->rsp);
        return true;
    }

    bool OnAfter() override {
        http::send(m_request.r, m_respond.header, m_respond.body.c_str(), m_respond.body.size());
        return true;
    }

protected:
    HttpRequest m_request;
    HttpRespond m_respond;
};