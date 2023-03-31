/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_CURL

#include "async/co_async/curl/co_curl.h"
#include "async/co_async/promise.h"

namespace co_async {
namespace curl {

std::pair<int, std::shared_ptr<async::curl::CurlParser>> get(const std::string& url, const std::unordered_map<std::string, std::string>& headers, const TimeOut& t) {
    auto res = co_async::promise([&url, &headers](co_async::Resolve resolve, co_async::Reject reject) {
        async::curl::get(url, [resolve](async::curl::CurlParserPtr parser) {
            resolve(parser);
        }, headers);
    }, t());

    std::pair<int, std::shared_ptr<async::curl::CurlParser>> ret = std::make_pair(res.first, nullptr);
    if (co_async::checkOk(res)) {
        ret.second = co_async::getOk<async::curl::CurlParser>(res);
    }

    return ret;
}

int get(const std::string& url, std::string& body, const std::unordered_map<std::string, std::string>& headers, const TimeOut& t) {
    auto ret = get(url, headers, t);
    if (co_async::checkOk(ret)) {
        ret.second->swapValue(body);
    }

    return ret.first;
}

std::pair<int, std::shared_ptr<async::curl::CurlParser>> post(const std::string& url, const std::string& content, const std::unordered_map<std::string, std::string>& headers, const TimeOut& t) {
    auto res = co_async::promise([&url, &content, &headers](co_async::Resolve resolve, co_async::Reject reject) {
        async::curl::post(url, content, [resolve](async::curl::CurlParserPtr parser) {
            resolve(parser);
        }, headers);
    }, t());

    std::pair<int, std::shared_ptr<async::curl::CurlParser>> ret = std::make_pair(res.first, nullptr);
    if (co_async::checkOk(res)) {
        ret.second = co_async::getOk<async::curl::CurlParser>(res);
    }

    return ret;
}

int post(const std::string& url, const std::string& content, std::string& rspBody, const std::unordered_map<std::string, std::string>& headers, const TimeOut& t) {
    auto ret = post(url, content, headers, t);
    if (co_async::checkOk(ret)) {
        ret.second->swapValue(rspBody);
    }

    return ret.first;
}

}
}

#endif