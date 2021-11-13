/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#include "common/co_async/co_io.h"
#include "common/coroutine/coroutine.hpp"
#include "common/co_bridge/co_bridge.h"
#include <vector>
#include <assert.h>

namespace co_async { 

//////////////////////////////////////////////////////////////////////////

struct co_parallel_result {
    bool timeout_flag = false;
    int fns = 0;
};

int parallel(const std::initializer_list<fn_cb>& fns) {
    if (fns.size() == 0) {
        return co_bridge::E_CO_RETURN_ERROR;
    }

    unsigned int co_id = Coroutine::curid();
    if (co_id == M_MAIN_COROUTINE_ID) {
        assert(false);
        return co_bridge::E_CO_RETURN_ERROR;
    }

    co_parallel_result* result = new co_parallel_result;
    result->fns = fns.size();

    int64_t unique_id = co_bridge::genUniqueId();
    int64_t timer_id = co_bridge::addTimer(10 * 1000, [result, co_id, unique_id]() {
        result->timeout_flag = true;
        co_bridge::rmUniqueId(unique_id);
        Coroutine::resume(co_id);
    });

    auto done = [unique_id, timer_id, co_id] () {
        if (!co_bridge::rmUniqueId(unique_id)) {
            return;
        }
        co_bridge::rmTimer(timer_id);
        Coroutine::resume(co_id);
    };

    next_cb next = [result, done]() {
        result->fns -= 1;
        if (result->fns == 0) {
            done();
        }
    };

    for (auto& f : fns) {
        f(next);
    }

    co_bridge::addUniqueId(unique_id);
    Coroutine::yield();

    int ret = co_bridge::E_CO_RETURN_OK;
    if (result->timeout_flag) {
        ret = co_bridge::E_CO_RETURN_TIMEOUT;
    }
    
    delete result;
    return ret;
}

/////////////////////////////////////////////////////////////////////////

struct co_series_result {
    next_cb next;
    std::vector<fn_cb> fns;
    size_t idx = 0;
    bool timeout_flag = false;
};

int series(const std::initializer_list<fn_cb>& fns) {
    if (fns.size() == 0) {
        return co_bridge::E_CO_RETURN_ERROR;
    }

    unsigned int co_id = Coroutine::curid();
    if (co_id == M_MAIN_COROUTINE_ID) {
        assert(false);
        return co_bridge::E_CO_RETURN_ERROR;
    }

    co_series_result* result = new co_series_result;
    for (auto& f : fns) {
        result->fns.push_back(f);
    }

    int64_t unique_id = co_bridge::genUniqueId();
    int64_t timer_id = co_bridge::addTimer(10 * 1000, [result, co_id, unique_id]() {
        result->timeout_flag = true;
        co_bridge::rmUniqueId(unique_id);
        Coroutine::resume(co_id);
    });

    auto done = [unique_id, timer_id, co_id] () {
        if (!co_bridge::rmUniqueId(unique_id)) {
            return;
        }
        co_bridge::rmTimer(timer_id);
        Coroutine::resume(co_id);
    };

    next_cb next = [result, done]() {
        result->idx++;
        if (result->idx == result->fns.size()) {
            done();
        }    
        else {
            result->fns[result->idx](result->next);
        }
    };

    result->next = next;
    result->fns[result->idx](next);

    co_bridge::addUniqueId(unique_id);
    Coroutine::yield();

    int ret = co_bridge::E_CO_RETURN_OK;
    if (result->timeout_flag) {
        ret = co_bridge::E_CO_RETURN_TIMEOUT;
    }
    
    delete result;
    return ret;
}

}