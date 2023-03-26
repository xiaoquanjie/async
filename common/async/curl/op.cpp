/*----------------------------------------------------------------
// Copyright 2023
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_CURL

#include "common/async/curl/data.h"
#include "common/async/curl/curl_parser.h"
#include "common/async/curl/callback.h"

namespace async {
namespace curl {

void opRequest(CurlCorePtr c, CurlReqDataPtr req) {
    do {
        auto curl = curl_easy_init();
        if (!curl) {
            curlLog("[error] failed to call curl_easy_init%s", "");
            break;
        }

        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
        curl_easy_setopt(curl, CURLOPT_URL, req->url.c_str());
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 120L);

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &req->rspBody);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, onWriteCb);

        if (req->method == METHOD_POST) {
            curl_easy_setopt(curl, CURLOPT_POST, 1);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, req->postBody.size());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req->postBody.c_str());
        }
        else if (req->method == METHOD_PUT) {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req->postBody.c_str());
        }

        // set header
        struct curl_slist* curl_headers = 0;
        curl_headers = curl_slist_append(curl_headers, "connection: keep-alive");
        for (const auto& header : req->headers) {
            curl_headers = curl_slist_append(curl_headers, (header.first + ": " + header.second).c_str());
        }
        if (curl_headers) {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);
        }

        req->header = curl_headers;

        auto code = curl_multi_add_handle(c->curlm, curl);
        if (code != CURLM_OK) {
            curlLog("[error] failed to call curl_multi_add_handle: %d", code);
            break;
        }

        c->curlMap[curl] = req;
        return;
    } while (false);
    
    auto rsp = std::make_shared<CurlRspData>();
    rsp->req = req;
    rsp->parser = std::make_shared<CurlParser>("", CURLE_FAILED_INIT, 0);
    onPushRsp(rsp, req->tData);
}

}
}

#endif