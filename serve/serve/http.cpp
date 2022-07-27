/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_NET

#include "serve/serve/http.h"
#include "common/net/http_listener.h"
#include "common/transaction/transaction_mgr.h"
#include <memory>

namespace http {

typedef std::shared_ptr<net::HttpListener> HttpListenerPtr;

std::unordered_map<HttpListenerPtr, uint32_t> gHttpListener;

bool init(void* event_base, const LinkInfo& link) {
    for (auto item : link.itemVec) {
        auto listener = std::make_shared<net::HttpListener>(event_base);
        if (!listener->listen(item.ip, item.port)) {
            return false;
        }

        uint32_t id = item.w.id;

        // 设置回调
        listener->setDataCb([id](net::HttpListener*, void* r, const char* body, uint32_t len) {
            // 解析数据
            HttpMsg message;
            message.request = r;
            auto url = net::HttpListener::getUrlPath(r);
            if (url) {
                message.url = url;
            }
            auto host = net::HttpListener::getRemoteHost(r);
            if (host) {
                message.host = host;
            }
            auto query = net::HttpListener::getUrlParam(r);
            if (query) {
                message.query = query;
            }
            if (body) {
                message.body.append(body, len);
            }
            
            trans_mgr::handle(id, message.url, (char*)&message, 0, 0);
        });

        gHttpListener[listener] = id;
    }

    return true;
}

void update(uint32_t curTime) {
    for (auto& item : gHttpListener) {
        item.first->update(curTime);
    }
}

void send(void* request, const char* buf, uint32_t len) {
    net::HttpListener::send(request, buf, len);
}

void send(void* request, const std::unordered_map<std::string, std::string>& header, const char* buf, uint32_t len) {
    net::HttpListener::send(request, header, buf, len);
}

const char* decodeUri(const char* s) {
    return net::HttpListener::decodeUri(s);
}

}

#endif