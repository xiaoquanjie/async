/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_CURL

#include "common/co_async/co_curl.h"
#include "common/coroutine/coroutine.hpp"

namespace co_async {
namespace curl {

int g_wait_time = E_WAIT_TEN_SECOND;

int get_wait_time() {
    return g_wait_time;
}

void set_wait_time(int wait_time) {
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
        return E_CO_RETURN_ERROR;
    }

    int64_t unique_id = co_async::gen_unique_id();
    co_curl_result* result = new co_curl_result;

    int64_t timer_id = co_async::add_timer(g_wait_time, [result, co_id, unique_id]() {
        result->timeout_flag = true;
        co_async::rm_unique_id(unique_id);
        Coroutine::resume(co_id);
    });

    async::curl::get(url, headers, [result, timer_id, co_id, unique_id](int curl_code, int rsp_code, std::string& body) {
        if (!co_async::rm_unique_id(unique_id)) {
            return;
        }
        co_async::rm_timer(timer_id);
        result->curl_code = curl_code;
        result->rsp_code = rsp_code;
        result->body.swap(body);
        Coroutine::resume(co_id);
    });

    co_async::add_unique_id(unique_id);
    Coroutine::yield();

    int ret = E_CO_RETURN_OK;
    if (result->timeout_flag) {
        ret = E_CO_RETURN_TIMEOUT;
    }
    else {
        cb(result->curl_code, result->rsp_code, result->body);
    }

    delete result;
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
        return E_CO_RETURN_ERROR;
    }

    int64_t unique_id = co_async::gen_unique_id();
    bool timeout_flag = false;

    int64_t timer_id = co_async::add_timer(g_wait_time, [&timeout_flag, co_id, unique_id]() {
        timeout_flag = true;
        co_async::rm_unique_id(unique_id);
        Coroutine::resume(co_id);
    });

    co_curl_result* result = new co_curl_result;
    async::curl::post(url, content, headers, [result, timer_id, co_id, unique_id](int curl_code, int rsp_code, std::string& body) {
        if (!co_async::rm_unique_id(unique_id)) {
            return;
        }
        co_async::rm_timer(timer_id);
        result->curl_code = curl_code;
        result->rsp_code = rsp_code;
        result->body.swap(body);
        Coroutine::resume(co_id);
    });

    co_async::add_unique_id(unique_id);
    Coroutine::yield();

    if (timeout_flag) {
        return E_CO_RETURN_TIMEOUT;
    }

    cb(result->curl_code, result->rsp_code, result->body);
    delete result;
    return E_CO_RETURN_OK;
}

}
}

#endif