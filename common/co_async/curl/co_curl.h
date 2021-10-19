/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include "common/async/curl/async_curl.h"
#include "common/co_async/co_io.h"

namespace co_async {
namespace curl {

int get_wait_time();

void set_wait_time(int wait_time);

int get(const std::string& url, async::curl::async_curl_cb cb);

int get(const std::string &url, const std::map<std::string, std::string>& headers, async::curl::async_curl_cb cb);

int post(const std::string& url, const std::string& content, async::curl::async_curl_cb cb);

int post(const std::string& url, const std::string& content, const std::map<std::string, std::string>& headers, async::curl::async_curl_cb cb);

}
}