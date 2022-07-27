/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
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
        m_request.r = msg->request;
        m_request.url.swap(msg->url);
        m_request.host.swap(msg->host);
        m_request.squery.swap(msg->query);
        m_request.body.swap(msg->body);

        std::vector<std::string> vec;
        split(m_request.squery, "&", vec);
        for (auto& p : vec) {
            std::vector<std::string> pVec;
            split(p, "=", pVec);
            if (pVec.size() != 2) {
                continue;
            }
            if (pVec[0].empty()) {
                continue;
            }
            if (pVec[1].empty()) {
                continue;
            }
            std::string k = http::decodeUri(pVec[0].c_str());
            m_request.query[k] = http::decodeUri(pVec[1].c_str());
        }
        return true;
    }

    bool OnBefore() override {
        return true;
    }

    bool OnAfter() override {
        http::send(m_request.r, m_respond.header, m_respond.body.c_str(), m_respond.body.size());
        return true;
    }

protected:
    Request m_request;
    Respond m_respond;
};