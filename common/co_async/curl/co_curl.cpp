/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_CURL

#include "common/co_async/curl/co_curl.h"
#include "common/coroutine/coroutine.hpp"
#include "common/co_bridge/co_bridge.h"

namespace co_async {
namespace curl {

int g_wait_time = co_bridge::E_WAIT_TEN_SECOND;

int getWaitTime() {
    return g_wait_time;
}

void setWaitTime(int wait_time) {
    g_wait_time = wait_time;
}

struct co_curl_result {
    bool timeout_flag = false;
    int curl_code;
    int rsp_code;
    std::string body; 
};

/////////////////////////////////////////////////////////////////

int get(const std::string& url, async::curl::async_curl_cb cb) {
    std::map<std::string, std::string> header;
    return co_async::curl::get(url, header, cb);
}

int get(const std::string &url, const std::map<std::string, std::string>& headers, async::curl::async_curl_cb cb) {
    unsigned int co_id = Coroutine::curid();
    if (co_id == M_MAIN_COROUTINE_ID) {
        assert(false);
        return co_bridge::E_CO_RETURN_ERROR;
    }

    int64_t unique_id = co_bridge::genUniqueId();
    auto result = std::make_shared<co_curl_result>();

    int64_t timer_id = co_bridge::addTimer(g_wait_time, [result, co_id, unique_id]() {
        result->timeout_flag = true;
        co_bridge::rmUniqueId(unique_id);
        Coroutine::resume(co_id);
    });

    async::curl::get(url, headers, [result, timer_id, co_id, unique_id](int curl_code, int rsp_code, std::string& body) {
        if (!co_bridge::rmUniqueId(unique_id)) {
            return;
        }
        co_bridge::rmTimer(timer_id);
        result->curl_code = curl_code;
        result->rsp_code = rsp_code;
        result->body.swap(body);
        Coroutine::resume(co_id);
    });

    co_bridge::addUniqueId(unique_id);
    Coroutine::yield();

    int ret = co_bridge::E_CO_RETURN_OK;
    if (result->timeout_flag) {
        ret = co_bridge::E_CO_RETURN_TIMEOUT;
    }
    else {
        cb(result->curl_code, result->rsp_code, result->body);
    }

    return ret;
}

int post(const std::string& url, const std::string& content, async::curl::async_curl_cb cb) {
    std::map<std::string, std::string> header;
    return co_async::curl::post(url, content, header, cb);
}

int post(const std::string& url, const std::string& content, const std::map<std::string, std::string>& headers, async::curl::async_curl_cb cb) {
    unsigned int co_id = Coroutine::curid();
    if (co_id == M_MAIN_COROUTINE_ID) {
        assert(false);
        return co_bridge::E_CO_RETURN_ERROR;
    }

    int64_t unique_id = co_bridge::genUniqueId();
    bool timeout_flag = false;

    int64_t timer_id = co_bridge::addTimer(g_wait_time, [&timeout_flag, co_id, unique_id]() {
        timeout_flag = true;
        co_bridge::rmUniqueId(unique_id);
        Coroutine::resume(co_id);
    });

    auto result = std::make_shared<co_curl_result>();
    async::curl::post(url, content, headers, [result, timer_id, co_id, unique_id](int curl_code, int rsp_code, std::string& body) {
        if (!co_bridge::rmUniqueId(unique_id)) {
            return;
        }
        co_bridge::rmTimer(timer_id);
        result->curl_code = curl_code;
        result->rsp_code = rsp_code;
        result->body.swap(body);
        Coroutine::resume(co_id);
    });

    co_bridge::addUniqueId(unique_id);
    Coroutine::yield();

    if (timeout_flag) {
        return co_bridge::E_CO_RETURN_TIMEOUT;
    }

    cb(result->curl_code, result->rsp_code, result->body);
    return co_bridge::E_CO_RETURN_OK;
}

}
}

#endif