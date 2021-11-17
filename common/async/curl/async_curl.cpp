/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_CURL

#include "common/async/curl/async_curl.h"
#include <queue>
#include <string.h>
#include <curl/curl.h>

// 定义该宏会打开调试日志
//#define M_ASYNC_CURL_CLIENT_LOG (1)

#ifdef M_ASYNC_CURL_CLIENT_LOG
#include <stdio.h>
    #define ASYNC_CURL_CLIENT_LOG printf
#else
    #define ASYNC_CURL_CLIENT_LOG(...) 
#endif

namespace async {
namespace curl {

enum HTTP_METHOD { 
    METHOD_GET = 0,
    METHOD_POST,
    METHOD_PUT,
};

struct curl_custom_data {
    struct curl_slist* curl_headers = 0;
    std::string respond_body;
    async_curl_cb cb;
    CURL* curl = 0;

    ~curl_custom_data() {
        if (curl_headers) {
            curl_slist_free_all(curl_headers);
        }
    }
};

struct curl_respond_data {
    CURL* curl;
    int curl_code;
    int rsp_code;
};

// 全局变量
struct curl_global_data {
    bool init = false;
    CURLM* curlm = 0;
    std::map<CURL*, struct curl_custom_data*> curl_custom_map;
    std::queue<curl_custom_data*> request_queue;
    std::queue<curl_respond_data> respond_queue;
    bool flag;
    std::function<void(std::function<void()>)> thr_func;
    curl_global_data() : flag(false) {}
};

static curl_global_data g_curl_global_data;

////////////////////////////////////////////////////////////////////////////////////

bool local_curl_global_init() {
    if (g_curl_global_data.init) {
        return true;
    }

    auto code = curl_global_init(CURL_GLOBAL_ALL);
    if (code != CURLE_OK) {
        printf("[curl] failed to call curl_global_init: %d\n", code);
        return false;
    }

    g_curl_global_data.curlm = curl_multi_init();
    if (!g_curl_global_data.curlm) {
        printf("[curl] failed to call curl_multi_init: %d\n", errno);
        return false;
    }

    g_curl_global_data.init = true;
    return true;
}

void local_curl_global_cleanup() {
    if (g_curl_global_data.init) {
        curl_global_cleanup();
    }
}

size_t on_thread_curl_writer(void *buffer, size_t size, size_t count, void* param) {
    std::string* s = static_cast<std::string*>(param);
    if (nullptr == s) {
        return 0;
    }

    s->append(reinterpret_cast<char*>(buffer), size * count);
    return size * count;
}

// 创建一个curl
curl_custom_data* local_create_curl_custom_data(int method,
                                    const std::string &url,
                                    const std::map<std::string, std::string> &headers,
                                    const std::string &content)
{
    CURL* curl = curl_easy_init();
    if (!curl) {
        printf("[curl] failed to call curl_easy_init: %d, method:%d, url:%s\n", errno, method, url.c_str());
        return 0;
    }

    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 120L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, on_thread_curl_writer);

    if (method == METHOD_POST) {
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, content.size());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, content.c_str());
    }
    else if (method == METHOD_PUT) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, content.c_str());
    }

    // set header
    struct curl_slist* curl_headers = 0;
    curl_headers = curl_slist_append(curl_headers, "connection: keep-alive");
    for (const auto& header : headers) {
        curl_headers = curl_slist_append(curl_headers, (header.first + ": " + header.second).c_str());
    }
    if (curl_headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);
    }

    // create a new curl_custom_data
    curl_custom_data* data = new curl_custom_data;
    data->curl_headers = curl_headers;
    data->curl = curl;
    
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data->respond_body);
    return data;
}

int thread_curl_multi_select(CURLM* curlm) {
    fd_set fdread;
    fd_set fdwrite;
    fd_set fdexcep;
    int maxfd = -1;

    FD_ZERO(&fdread);
    FD_ZERO(&fdwrite);
    FD_ZERO(&fdexcep);

    auto code = curl_multi_fdset(curlm, &fdread, &fdwrite, &fdexcep, &maxfd);
    if (CURLM_OK != code) {
        printf("[curl] failed to call curl_multi_fdset: %d\n", code);
        return 0;
    }

    if (-1 == maxfd) {
        return 0;
    }

    timeval timeout;
    memset(&timeout, 0, sizeof(timeout));
    return select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);
}

void thread_curl_finish(CURLM* curlm, std::queue<curl_respond_data>& queue) {
    CURLMsg* curl_msg = nullptr;
    int msgs_left = 0;
    while ((curl_msg = curl_multi_info_read(curlm, &msgs_left))) {
        if (CURLMSG_DONE != curl_msg->msg) {
            continue;
        }

        int32_t response_code = 0;
        CURL* curl = (CURL*)curl_msg->easy_handle;
        auto curl_code = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (CURLE_OK != curl_code) {
            printf("[curl] failed to call curl_easy_getinfo: %d\n", curl_code);
        }

        // push to queue
        queue.push({curl, curl_msg->data.result, response_code});

        // curl_multi_remove_handle -> curl_easy_cleanup
        curl_multi_remove_handle(curlm, curl);
        break;
    }
}

void thread_curl_process(CURLM* curlm, std::queue<curl_respond_data>& queue, bool* flag) {
    // int selected_count = thread_curl_multi_select(curlm);
    // if (selected_count == -1) {
    //     printf("curl: failed to call select: %d\n", errno);
    //     *flag = false;
    //     return;
    // }

    /* timeout or readable/writable sockets */
    int still_running = 0;
    do {
        curl_multi_wait(curlm, NULL, 0, 0, NULL);
        auto code = curl_multi_perform(curlm, &still_running);
        if (code != CURLM_OK) {
            printf("[curl] failed to call curl_multi_perform: %d\n", code);
            break;
        }
    } while (still_running);

    thread_curl_finish(curlm, queue);
    *flag = false;
}

void local_curl_request(int method,
                        const std::string &url,
                        const std::map<std::string, std::string> &headers,
                        const std::string &content,
                        async_curl_cb cb)
{
    local_curl_global_init();
    curl_custom_data* data = local_create_curl_custom_data(method, url, headers, content);
    data->cb = cb;
    g_curl_global_data.request_queue.push(data);
}

void thread_curl_process_cb(void* c, void* q, bool* flag) {
    CURLM* curlm = (CURLM*)c;
    std::queue<curl_respond_data>* queue = (std::queue<curl_respond_data>*)q;
    thread_curl_process(curlm, *queue, flag);
}

void setThreadFunc(std::function<void(std::function<void()>)> f) {
    g_curl_global_data.thr_func = f;
}

inline std::function<void()> get_thread_func() {
    g_curl_global_data.flag = true;
    auto f = std::bind(thread_curl_process_cb, 
        (void*)g_curl_global_data.curlm, 
        (void*)(&g_curl_global_data.respond_queue),
        &g_curl_global_data.flag);
    return f;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void get(const std::string &url, async_curl_cb cb) {
    std::map<std::string, std::string> headers;
    get(url, headers, cb);
}

void get(const std::string &url, const std::map<std::string, std::string>& headers, async_curl_cb cb) {
    local_curl_request(METHOD_GET, url, headers, "", cb);
}

void post(const std::string& url, const std::string& content, async_curl_cb cb) {
    std::map<std::string, std::string> headers;
    post(url, content, headers, cb);
}

void post(const std::string& url, const std::string& content, const std::map<std::string, std::string>& headers, async_curl_cb cb) {
    local_curl_request(METHOD_POST, url, headers, content, cb);
}

bool loop() {
    if (!g_curl_global_data.init) {
        return false;
    }
    if (g_curl_global_data.flag) {
        return true;
    }

    // 是否有任务
    bool has_task = false;

    // 交换出结果队列
    std::queue<curl_respond_data> respond_queue;
    g_curl_global_data.respond_queue.swap(respond_queue);

    // 处理请求
    while (!g_curl_global_data.request_queue.empty()) {
        curl_custom_data *req = std::move(g_curl_global_data.request_queue.front());
        g_curl_global_data.request_queue.pop();
        // 添加到CURLM中
        auto code = curl_multi_add_handle(g_curl_global_data.curlm, req->curl);
        if (code != CURLM_OK) {
            curl_easy_cleanup(req->curl);
            delete req;
            printf("[curl] failed to call curl_multi_add_handle: %d\n", code);
            continue;
        }

        g_curl_global_data.curl_custom_map[req->curl] = req;
    }

    // 执行任务
    if (!g_curl_global_data.curl_custom_map.empty()) {
        // 有任务
        has_task = true;
        if (g_curl_global_data.thr_func) {
            g_curl_global_data.thr_func(get_thread_func());
        }
        else {
            get_thread_func()();
        }
        ASYNC_CURL_CLIENT_LOG("curl: request count:%d simultaneously\n", (int)g_curl_global_data.curl_custom_map.size());
    }

    // 运行结果
    while (!respond_queue.empty()) {
        // 取出结果
        curl_respond_data rsp = std::move(respond_queue.front());
        respond_queue.pop();
        auto iter = g_curl_global_data.curl_custom_map.find(rsp.curl);
        if (iter != g_curl_global_data.curl_custom_map.end()) {
            // 回调通知
            iter->second->cb(rsp.curl_code, rsp.rsp_code, iter->second->respond_body);
            delete (iter->second);
            g_curl_global_data.curl_custom_map.erase(iter);
            ASYNC_CURL_CLIENT_LOG("curl: finish request: %d\n", (int)g_curl_global_data.curl_custom_map.size());
        }
        // release
        curl_easy_cleanup(rsp.curl);
    }

    return has_task;
}

}
}

#endif