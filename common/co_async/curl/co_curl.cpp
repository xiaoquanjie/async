/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_CURL

#include "common/co_async/curl/co_curl.h"
#include "common/co_async/promise.h"

namespace co_async {
namespace curl {

/////////////////////////////////////////////////////////////////

std::pair<int, std::shared_ptr<CurlResult>> get(const std::string& url, const TimeOut& t) {
    std::map<std::string, std::string> header;
    return get(url, header, t);
}

std::pair<int, std::shared_ptr<CurlResult>> get(const std::string& url, const std::map<std::string, std::string>& headers, const TimeOut& t) {
    auto res = co_async::promise([&url, &headers](co_async::Resolve resolve, co_async::Reject reject) {
        async::curl::get(url, headers, [resolve](int curl_code, int rsp_code, std::string& body) {
            auto p = std::make_shared<CurlResult>();
            p->curlCode = curl_code;
            p->resCode = rsp_code;
            p->body.swap(body);
            resolve(p);
        });
    }, t());

    std::pair<int, std::shared_ptr<CurlResult>> ret = std::make_pair(res.first, nullptr);
    if (co_async::checkOk(res)) {
        ret.second = co_async::getOk<CurlResult>(res);
    }

    return ret;
}

int get(const std::string& url, std::string& body, const TimeOut& t) {
    std::map<std::string, std::string> header;
    return get(url, header, body, t);
}

int get(const std::string& url, const std::map<std::string, std::string>& headers, std::string& body, const TimeOut& t) {
    auto ret = get(url, headers, t);
    if (co_async::checkOk(ret)) {
        ret.second->body.swap(body);
    }

    return ret.first;
}

std::pair<int, std::shared_ptr<CurlResult>> post(const std::string& url, const std::string& content, const TimeOut& t) {
    std::map<std::string, std::string> header;
    return post(url, content, header, t);
}

std::pair<int, std::shared_ptr<CurlResult>> post(const std::string& url, const std::string& content, const std::map<std::string, std::string>& headers, const TimeOut& t) {
    auto res = co_async::promise([&url, &content, &headers](co_async::Resolve resolve, co_async::Reject reject) {
        async::curl::post(url, content, headers, [resolve](int curl_code, int rsp_code, std::string& body) {
            auto p = std::make_shared<CurlResult>();
            p->curlCode = curl_code;
            p->resCode = rsp_code;
            p->body.swap(body);
            resolve(p);
        });
    }, t());

    std::pair<int, std::shared_ptr<CurlResult>> ret = std::make_pair(res.first, nullptr);
    if (co_async::checkOk(res)) {
        ret.second = co_async::getOk<CurlResult>(res);
    }

    return ret;
}

int post(const std::string& url, const std::string& content, std::string& rspBody, const TimeOut& t) {
    std::map<std::string, std::string> headers;
    return post(url, content, headers, rspBody, t);
}

int post(const std::string& url, const std::string& content, const std::map<std::string, std::string>& headers, std::string& rspBody, const TimeOut& t) {
    auto ret = post(url, content, headers, t);
    if (co_async::checkOk(ret)) {
        ret.second->body.swap(rspBody);
    }

    return ret.first;
}

}
}

#endif