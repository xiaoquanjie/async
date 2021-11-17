/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include <functional>
#include <map>

namespace async {

namespace curl {

// CURLcode, response_code, body
typedef std::function<void(int, int, std::string&)> async_curl_cb;

void get(const std::string &url, async_curl_cb);

void get(const std::string &url, const std::map<std::string, std::string>& headers, async_curl_cb cb);

void post(const std::string& url, const std::string& content, async_curl_cb cb);

void post(const std::string& url, const std::string& content, const std::map<std::string, std::string>& headers, async_curl_cb cb);

bool loop();

void setThreadFunc(std::function<void(std::function<void()>)>);

// 设置日志接口, 要求是线程安全的
void setLogFunc(std::function<void(const char*)> cb);

void log(const char* format, ...);

}// curl
}// async


