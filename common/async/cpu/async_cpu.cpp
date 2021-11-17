/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#include "common/async/cpu/async_cpu.h"
#include <queue>
#include <mutex>
#include <assert.h>
#include <stdarg.h>
#include <memory>

namespace async {
namespace cpu {

struct cpu_custom_data {
    async_cpu_op op;
    void* user_data = 0;
    async_cpu_cb cb;
};

typedef std::shared_ptr<cpu_custom_data> cpu_custom_data_ptr;

struct cpu_respond_data {
    cpu_custom_data_ptr req;
    int64_t result = 0;
};

struct cpu_global_data {
    std::function<void(std::function<void()>)> thr_func;
    std::queue<cpu_custom_data_ptr> request_queue;
    std::queue<cpu_respond_data> respond_queue;
    std::mutex respond_mutext;
    uint64_t req_task_cnt = 0;
    uint64_t rsp_task_cnt = 0;
};

cpu_global_data g_cpu_global_data;

time_t g_last_statistics_time = 0;

// 日志输出接口
std::function<void(const char*)> g_log_cb = [](const char* data) {
    static std::mutex s_mutex;
    s_mutex.lock();
    printf("[async_cpu] %s\n", data);
    s_mutex.unlock();
};

void log(const char* format, ...) {
    if (!g_log_cb) {
        return;
    }

    char buf[1024] = { 0 };
    va_list ap;
    va_start(ap, format);
    vsprintf(buf, format, ap);
    g_log_cb(buf);
}

////////////////////////////////////////////////////////////////////////////

void thread_cpu_process(cpu_custom_data_ptr req, cpu_global_data* global_data) {
    cpu_respond_data rsp_data;
    rsp_data.req = req;
    // 执行计算
    rsp_data.result = req->op(req->user_data);
    // 存入队例
    global_data->respond_mutext.lock();
    global_data->respond_queue.emplace(rsp_data);
    global_data->respond_mutext.unlock();
}

inline std::function<void()> get_thread_func() {
    // 取一条
    assert(!g_cpu_global_data.request_queue.empty());
    cpu_custom_data_ptr req_data = g_cpu_global_data.request_queue.front();
    g_cpu_global_data.request_queue.pop();

    auto f = std::bind(thread_cpu_process, req_data, &g_cpu_global_data);
    return f;
}

/////////////////////////////////////////////////////////////////////////////

void execute(async_cpu_op op, void* user_data, async_cpu_cb cb) {
    cpu_custom_data_ptr req = std::make_shared<cpu_custom_data>();
    req->op = op;
    req->user_data = user_data;
    req->cb = cb;
    g_cpu_global_data.request_queue.push(req);
    g_cpu_global_data.req_task_cnt++;
}

void setThreadFunc(std::function<void(std::function<void()>)> f) {
    g_cpu_global_data.thr_func = f;
}

void setLogFunc(std::function<void(const char*)> cb) {
    g_log_cb = cb;
}

void statistics() {
    if (!g_log_cb) {
        return;
    }

    time_t now = 0;
    time(&now);
    if (now - g_last_statistics_time <= 120) {
        return;
    }

    g_last_statistics_time = now;
    log("[statistics] cur_task:%d, req_task:%d, rsp_task:%d",
        (g_cpu_global_data.req_task_cnt - g_cpu_global_data.rsp_task_cnt),
        g_cpu_global_data.req_task_cnt,
        g_cpu_global_data.rsp_task_cnt);
}

void local_process_respond() {
    if (g_cpu_global_data.respond_queue.empty()) {
        return;
    }

    // 交换队列
    std::queue<cpu_respond_data> respond_queue;
    g_cpu_global_data.respond_mutext.lock();
    respond_queue.swap(g_cpu_global_data.respond_queue);
    g_cpu_global_data.respond_mutext.unlock();

    // 运行结果
    while (!respond_queue.empty()) {
        g_cpu_global_data.rsp_task_cnt++;
        cpu_respond_data rsp = std::move(respond_queue.front());
        respond_queue.pop();
        rsp.req->cb(rsp.result, rsp.req->user_data);
    }
}

void local_process_task() {
    if (!g_cpu_global_data.request_queue.empty()) {
        if (g_cpu_global_data.thr_func) {
            g_cpu_global_data.thr_func(get_thread_func());
        }
        else {
            get_thread_func()();
        }
    }
}

bool loop() {
    local_process_respond();
    local_process_task();
    statistics();

    // 是否有任务
    bool has_task = false;
    if (g_cpu_global_data.req_task_cnt != g_cpu_global_data.rsp_task_cnt) {
        has_task = true;
    }
    return has_task;
}

}
}