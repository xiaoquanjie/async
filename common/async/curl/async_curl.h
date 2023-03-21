/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include <functional>
#include <map>

#define CUR_CURL_VERSION (2)

namespace async {
namespace curl {

// CURLcode, response_code, body
typedef std::function<void(int, int, std::string&)> async_curl_cb;

void get(const std::string &url, async_curl_cb);

void get(const std::string &url, const std::map<std::string, std::string>& headers, async_curl_cb cb);

void post(const std::string& url, const std::string& content, async_curl_cb cb);

void post(const std::string& url, const std::string& content, const std::map<std::string, std::string>& headers, async_curl_cb cb);

bool loop(uint32_t curTime);

void setThreadFunc(std::function<void(std::function<void()>)>);

void setMaxConnection(uint32_t c);

}// curl
}// async


