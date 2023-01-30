/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#include "serve/serve/serve.h"
#include "serve/serve/http_transaction.h"
#include "common/log.h"

class GetNameTransaction : public HttpTransaction {
public:
    int OnRequest() {
        log("url:%s", m_request.url.c_str());
        log("host:%s", m_request.host.c_str());
        log("query:%s", m_request.squery.c_str());
        log("body:%s", m_request.body.c_str());

        // 回复
        m_respond.body = "hello";
        return 0;
    }
};

REGIST_HTTP_TRANSACTION(1, "/getname", GetNameTransaction);

int main(int argc, char** argv) {
    Serve srv;

    if (!srv.init(argc, argv)) {
        return -1;
    }

    // 添加中间件
    http::use(1, [](uint32_t, HttpRequest& req, HttpRespond&) {
        log("middle %s", req.url.c_str());
        req.squery += "&name=jack";
        // 消息往下传
        return false;
    });

    srv.start();
    return 0;
}