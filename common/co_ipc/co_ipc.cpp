#include "common/co_ipc/co_ipc.h"
#include "common/coroutine/coroutine.hpp"
#include "common/co_bridge/co_bridge.h"

namespace co_ipc {

int g_wait_time = co_bridge::E_WAIT_THIRTY_SECOND;

int getWaitTime() {
    return g_wait_time;
}

void setWaitTime(int wait_time) {
    g_wait_time = wait_time;
}

struct co_ipc_result {
    bool timeout_flag = false;
    const char* data = 0;
};

int send(std::function<void(int64_t)> io_send_func, const char** data) {
    unsigned int co_id = Coroutine::curid();
    if (co_id == M_MAIN_COROUTINE_ID) {
        assert(false);
        return co_bridge::E_CO_RETURN_ERROR;
    }

    int64_t sequence_id = co_bridge::gen_sequence_id();
    co_ipc_result* result = new co_ipc_result;

    int64_t timer_id = co_bridge::add_timer(g_wait_time, [result, co_id, sequence_id]() {
        result->timeout_flag = true;
        co_bridge::rm_sequence_id(sequence_id);
        Coroutine::resume(co_id);
    });

    // 执行真正的io发送
    io_send_func(sequence_id);

    co_bridge::add_sequence_id(sequence_id, timer_id, co_id, result);
    Coroutine::yield();

    int ret = co_bridge::E_CO_RETURN_OK;
    if (result->timeout_flag) {
        *data = 0;
        ret = co_bridge::E_CO_RETURN_TIMEOUT;
    }

    *data = result->data;
    delete result;
    return ret;
}

void recv(int64_t sequence_id, const char* data) {
    int64_t timer_id =0;
    unsigned int co_id = 0;
    co_ipc_result* result = 0;
    if (!co_bridge::rm_sequence_id(sequence_id, timer_id, co_id, (void**)&result)) {
        return;
    }

    result->data = data;
    co_bridge::rm_timer(timer_id);
    Coroutine::resume(co_id);
}

};