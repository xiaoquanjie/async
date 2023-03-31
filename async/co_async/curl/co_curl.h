/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include "async/async/curl/async_curl.h"
#include "async/co_async/comm.hpp"
#include <memory>

namespace co_async {
namespace curl {

std::pair<int, std::shared_ptr<async::curl::CurlParser>> get(const std::string& url, const std::unordered_map<std::string, std::string>& headers = {}, const TimeOut& t = TimeOut());

// @body是返回内容的body
int get(const std::string& url, std::string& body, const std::unordered_map<std::string, std::string>& headers = {}, const TimeOut& t = TimeOut());

std::pair<int, std::shared_ptr<async::curl::CurlParser>> post(const std::string& url, const std::string& content, const std::unordered_map<std::string, std::string>& headers = {}, const TimeOut& t = TimeOut());

// @rspBody是返回内容的body
int post(const std::string& url, const std::string& content, std::string& rspBody, const std::unordered_map<std::string, std::string>& headers = {}, const TimeOut& t = TimeOut());

}
}