/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#include "serve/serve/serve.h"
#include "serve/serve/http_transaction.h"

class GetNameTransaction : public HttpTransaction {
public:
    int OnRequest() {
        printf("url:%s\n", m_request.url.c_str());
        printf("host:%s\n", m_request.host.c_str());
        printf("query:%s\n", m_request.squery.c_str());
        printf("body:%s\n", m_request.body.c_str());

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

    srv.start();
    return 0;
}