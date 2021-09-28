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

namespace async {
namespace cpu {

struct cpu_custom_data {
    async_cpu_op op;
    void* user_data = 0;
    async_cpu_cb cb;
};

struct cpu_respond_data {
    cpu_custom_data* data;
    int64_t result = 0;
};

struct cpu_global_data {
    std::function<void(std::function<void()>)> thr_func;
    std::queue<cpu_custom_data*> request_queue;
    std::queue<cpu_respond_data> respond_queue;
    std::mutex respond_mutext;
    uint64_t req_task_cnt = 0;
    uint64_t rsp_task_cnt = 0;
};

static cpu_global_data g_cpu_global_data;

////////////////////////////////////////////////////////////////////////////

void thread_cpu_process(cpu_custom_data* req, cpu_global_data* global_data) {
    cpu_respond_data rsp_data;
    rsp_data.data = req;
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
    cpu_custom_data* req_data = g_cpu_global_data.request_queue.front();
    g_cpu_global_data.request_queue.pop();

    auto f = std::bind(thread_cpu_process, req_data, &g_cpu_global_data);
    return f;
}

/////////////////////////////////////////////////////////////////////////////

void execute(async_cpu_op op, void* user_data, async_cpu_cb cb) {
    cpu_custom_data* req = new cpu_custom_data;
    req->op = op;
    req->user_data = user_data;
    req->cb = cb;
    g_cpu_global_data.request_queue.push(req);
    g_cpu_global_data.req_task_cnt++;
}

void set_thread_func(std::function<void(std::function<void()>)> f) {
    g_cpu_global_data.thr_func = f;
}

bool loop() {
    // 是否有任务
    bool has_task = false;
    if (g_cpu_global_data.req_task_cnt != g_cpu_global_data.rsp_task_cnt) {
        has_task = true;
    }
    if (!g_cpu_global_data.request_queue.empty()) {
        if (g_cpu_global_data.thr_func) {
            g_cpu_global_data.thr_func(get_thread_func());
        }
        else {
            get_thread_func()();
        }
    }

    // 运行结果
    if (!g_cpu_global_data.respond_queue.empty()) {
        // 交换队列
        std::queue<cpu_respond_data> respond_queue;
        g_cpu_global_data.respond_mutext.lock();
        respond_queue.swap(g_cpu_global_data.respond_queue);
        g_cpu_global_data.respond_mutext.unlock();

        while (!respond_queue.empty()) {
            g_cpu_global_data.rsp_task_cnt++;
            cpu_respond_data rsp = std::move(respond_queue.front());
            respond_queue.pop();
            rsp.data->cb(rsp.result, rsp.data->user_data);
            delete rsp.data;
        } 
    }

    return has_task;
}

}
}