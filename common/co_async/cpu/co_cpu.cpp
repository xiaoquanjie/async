/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#include "common/co_async/cpu/co_cpu.h"
#include "common/coroutine/coroutine.hpp"
#include "common/co_bridge/co_bridge.h"

namespace co_async {
namespace cpu {

int g_wait_time = co_bridge::E_WAIT_THIRTY_SECOND;

int getWaitTime() {
    return g_wait_time;
}

void setWaitTime(int wait_time) {
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
        async::cpu::log("[co_cpu] not running in coroutine");
        assert(false);
        return co_bridge::E_CO_RETURN_ERROR;
    }

    int64_t unique_id = co_bridge::genUniqueId();
    co_cpu_result* result = new co_cpu_result;

    int64_t timer_id = co_bridge::addTimer(g_wait_time, [result, co_id, unique_id]() {
        result->timeout_flag = true;
        co_bridge::rmUniqueId(unique_id);
        Coroutine::resume(co_id);
    });

    async::cpu::execute(op, user_data, [result, co_id, timer_id, unique_id](int64_t res, void*) {
        if (!co_bridge::rmUniqueId(unique_id)) {
            return;
        }
        co_bridge::rmTimer(timer_id);
        result->result = res;
        Coroutine::resume(co_id);
    });

    co_bridge::addUniqueId(unique_id);
    Coroutine::yield();

    int ret = co_bridge::E_CO_RETURN_OK;
    if (result->timeout_flag) {
        ret = co_bridge::E_CO_RETURN_TIMEOUT;
    }
    else {
        cb(result->result, user_data);
    }
    
    delete result;
    return ret;
}

}
}