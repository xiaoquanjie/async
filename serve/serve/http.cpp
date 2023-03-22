/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_NET

#include "serve/serve/http.h"
#include "common/net/http_listener.h"
#include "common/transaction/transaction_mgr.h"
#include <memory>

namespace http {

typedef std::shared_ptr<net::HttpListener> HttpListenerPtr;
std::unordered_map<HttpListenerPtr, uint32_t> gHttpListener;

typedef std::vector<std::function<bool(uint32_t, HttpRequest&, HttpRespond&)>> MiddleVecType;
std::unordered_map<uint32_t, MiddleVecType> gMiddleMap;

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
            HttpMsg msg;
            msg.req.r = r;
            auto url = net::HttpListener::getUrlPath(r);
            if (url) {
                msg.req.url = url;
            }
            auto host = net::HttpListener::getRemoteHost(r);
            if (host) {
                msg.req.host = host;
            }
            auto query = net::HttpListener::getUrlParam(r);
            if (query) {
                msg.req.squery = query;

                std::vector<std::string> vec;
                split(msg.req.squery, "&", vec);
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
                    std::string k = decodeUri(pVec[0].c_str());
                    msg.req.query[k] = decodeUri(pVec[1].c_str());
                }
            }
            if (body) {
                msg.req.body.append(body, len);
            }

            for (auto f : gMiddleMap[id]) {
                if (f(id, msg.req, msg.rsp)) {
                    // 直接回复
                    send(msg.req.r, msg.rsp.header, msg.rsp.body.c_str(), msg.rsp.body.size());
                    return;
                }
            }
            trans_mgr::handle(id, msg.req.url, (char*)&msg, 0, 0);
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

void use(uint32_t id, std::function<bool(uint32_t, HttpRequest&, HttpRespond&)> fn) {
    gMiddleMap[id].push_back(fn);
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