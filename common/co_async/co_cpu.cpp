/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#include "common/co_async/co_cpu.h"
#include "common/coroutine/coroutine.hpp"

namespace co_async {
namespace cpu {

int g_wait_time = E_WAIT_TWO_MINUTE;

int get_wait_time() {
    return g_wait_time;
}

void set_wait_time(int wait_time) {
    g_wait_time = wait_time;
}

struct co_cpu_result {
    bool timeout_flag = false;
    int64_t result = 0;
};

/////////////////////////////////////////////////

int execute(async::cpu::async_cpu_op op, void* user_data, async::cpu::async_cpu_cb cb) {
    unsigned int co_id = Coroutine::curid();
    if (co_id == M_MAIN_COROUTINE_ID) {
        assert(false);
        return E_CO_RETURN_ERROR;
    }

    int64_t unique_id = co_async::gen_unique_id();
    co_cpu_result* result = new co_cpu_result;

    int64_t timer_id = co_async::add_timer(g_wait_time, [result, co_id, unique_id]() {
        result->timeout_flag = true;
        co_async::rm_unique_id(unique_id);
        Coroutine::resume(co_id);
    });

    async::cpu::execute(op, user_data, [result, co_id, timer_id, unique_id](int64_t res, void*) {
        if (!co_async::rm_unique_id(unique_id)) {
            return;
        }
        co_async::rm_timer(timer_id);
        result->result = res;
        Coroutine::resume(co_id);
    });

    co_async::add_unique_id(unique_id);
    Coroutine::yield();

    int ret = E_CO_RETURN_OK;
    if (result->timeout_flag) {
        ret = E_CO_RETURN_TIMEOUT;
    }
    else {
        cb(result->result, user_data);
    }
    
    delete result;
    return ret;
}

}
}