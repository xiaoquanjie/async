/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_CURL

#include "common/co_async/curl/co_curl.h"
#include "common/co_async/promise.h"

namespace co_async {
namespace curl {

/////////////////////////////////////////////////////////////////

std::pair<int, std::shared_ptr<CurlResult>> get(const std::string& url, int timeOut) {
    std::map<std::string, std::string> header;
    return get(url, header, timeOut);
}

std::pair<int, std::shared_ptr<CurlResult>> get(const std::string& url, const std::map<std::string, std::string>& headers, int timeOut) {
    auto res = co_async::promise([&url, &headers](co_async::Resolve resolve, co_async::Reject reject) {
        async::curl::get(url, headers, [resolve](int curl_code, int rsp_code, std::string& body) {
            auto p = std::make_shared<CurlResult>();
            p->curlCode = curl_code;
            p->resCode = rsp_code;
            p->body.swap(body);
            resolve(p);
        });
    }, timeOut);

    std::pair<int, std::shared_ptr<CurlResult>> ret = std::make_pair(res.first, nullptr);
    if (co_async::checkOk(res)) {
        ret.second = co_async::getOk<CurlResult>(res);
    }

    return ret;
}

int get(const std::string& url, std::string& body, int timeOut) {
    std::map<std::string, std::string> header;
    return get(url, header, body, timeOut);
}

int get(const std::string& url, const std::map<std::string, std::string>& headers, std::string& body, int timeOut) {
    auto ret = get(url, headers, timeOut);
    if (co_async::checkOk(ret)) {
        ret.second->body.swap(body);
    }

    return ret.first;
}

std::pair<int, std::shared_ptr<CurlResult>> post(const std::string& url, const std::string& content, int timeOut) {
    std::map<std::string, std::string> header;
    return post(url, content, header, timeOut);
}

std::pair<int, std::shared_ptr<CurlResult>> post(const std::string& url, const std::string& content, const std::map<std::string, std::string>& headers, int timeOut) {
    auto res = co_async::promise([&url, &content, &headers](co_async::Resolve resolve, co_async::Reject reject) {
        async::curl::post(url, content, headers, [resolve](int curl_code, int rsp_code, std::string& body) {
            auto p = std::make_shared<CurlResult>();
            p->curlCode = curl_code;
            p->resCode = rsp_code;
            p->body.swap(body);
            resolve(p);
        });
    }, timeOut);

    std::pair<int, std::shared_ptr<CurlResult>> ret = std::make_pair(res.first, nullptr);
    if (co_async::checkOk(res)) {
        ret.second = co_async::getOk<CurlResult>(res);
    }

    return ret;
}

int post(const std::string& url, const std::string& content, std::string& rspBody, int timeOut) {
    std::map<std::string, std::string> headers;
    return post(url, content, headers, rspBody, timeOut);
}

int post(const std::string& url, const std::string& content, const std::map<std::string, std::string>& headers, std::string& rspBody, int timeOut) {
    auto ret = post(url, content, headers, timeOut);
    if (co_async::checkOk(ret)) {
        ret.second->body.swap(rspBody);
    }

    return ret.first;
}

}
}

#endif