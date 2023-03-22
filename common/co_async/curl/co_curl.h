/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include "common/async/curl/async_curl.h"
#include "common/co_async/comm.hpp"
#include <memory>

namespace co_async {
namespace curl {

struct CurlResult {
    int curlCode = 0;
    int resCode = 0;
    std::string body;
};

std::pair<int, std::shared_ptr<CurlResult>> get(const std::string& url, const TimeOut& t = TimeOut());

std::pair<int, std::shared_ptr<CurlResult>> get(const std::string& url, const std::map<std::string, std::string>& headers, const TimeOut& t = TimeOut());

// @body是返回内容的body
int get(const std::string& url, std::string& body, const TimeOut& t = TimeOut());

// @body是返回内容的body
int get(const std::string& url, const std::map<std::string, std::string>& headers, std::string& body, const TimeOut& t = TimeOut());

std::pair<int, std::shared_ptr<CurlResult>> post(const std::string& url, const std::string& content, const TimeOut& t = TimeOut());

std::pair<int, std::shared_ptr<CurlResult>> post(const std::string& url, const std::string& content, const std::map<std::string, std::string>& headers, const TimeOut& t = TimeOut());

// @rspBody是返回内容的body
int post(const std::string& url, const std::string& content, std::string& rspBody, const TimeOut& t = TimeOut());

int post(const std::string& url, const std::string& content, const std::map<std::string, std::string>& headers, std::string& rspBody, const TimeOut& t = TimeOut());

}
}