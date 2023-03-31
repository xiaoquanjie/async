/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include <functional>
#include <unordered_map>
#include "async/async/curl/curl_parser.h"

#define CUR_CURL_VERSION (2)

namespace async {
namespace curl {

// CURLcode, response_code, body
typedef std::function<void(int, int, std::string&)> async_curl_cb;

typedef std::function<void(CurlParserPtr parser)> async_curl_cb2;

void get(const std::string &url, async_curl_cb cb, const std::unordered_map<std::string, std::string>& headers = {});
void get(const std::string &url, async_curl_cb2 cb, const std::unordered_map<std::string, std::string>& headers = {});

void post(const std::string& url, const std::string& content, async_curl_cb cb, const std::unordered_map<std::string, std::string>& headers = {});
void post(const std::string& url, const std::string& content, async_curl_cb2 cb, const std::unordered_map<std::string, std::string>& headers = {});

bool loop(uint32_t curTime);

void setThreadFunc(std::function<void(std::function<void()>)>);

void setMaxConnection(uint32_t c);

}// curl
}// async


