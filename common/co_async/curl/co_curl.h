/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include "common/async/curl/async_curl.h"
#include <memory>

namespace co_async {
namespace curl {

struct CurlResult {
    int curlCode = 0;
    int resCode = 0;
    std::string body;
};

std::pair<int, std::shared_ptr<CurlResult>> get(const std::string& url, int timeOut = 10 * 1000);

std::pair<int, std::shared_ptr<CurlResult>> get(const std::string& url, const std::map<std::string, std::string>& headers, int timeOut = 10 * 1000);

// @body是返回内容的body
int get(const std::string& url, std::string& body, int timeOut = 10 * 1000);

// @body是返回内容的body
int get(const std::string& url, const std::map<std::string, std::string>& headers, std::string& body, int timeOut = 10 * 1000);

std::pair<int, std::shared_ptr<CurlResult>> post(const std::string& url, const std::string& content, int timeOut = 10 * 1000);

std::pair<int, std::shared_ptr<CurlResult>> post(const std::string& url, const std::string& content, const std::map<std::string, std::string>& headers, int timeOut = 10 * 1000);

// @rspBody是返回内容的body
int post(const std::string& url, const std::string& content, std::string& rspBody, int timeOut = 10 * 1000);

int post(const std::string& url, const std::string& content, const std::map<std::string, std::string>& headers, std::string& rspBody, int timeOut = 10 * 1000);

}
}