/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#include "common/co_async/co_io.h"
#include <unordered_set>
#include <assert.h>
#include "common/co_async/time_pool.h"
#include "common/coroutine/coroutine.hpp"
#include "common/async/redis/async_redis.h"
#include "common/async/mongo/async_mongo.h"
#include "common/async/curl/async_curl.h"
#include "common/async/cpu/async_cpu.h"
#include "common/async/mysql/async_mysql.h"

namespace co_async { 

int64_t g_unique_id = 1;
std::unordered_set<int64_t> g_unique_id_set;
TimerPool time_pool;

int64_t gen_unique_id() {
    return g_unique_id++;
}

void add_unique_id(int64_t id) {
    assert(g_unique_id_set.find(id) == g_unique_id_set.end());
    g_unique_id_set.insert(id);
}

bool rm_unique_id(int64_t id) {
    auto iter = g_unique_id_set.find(id);
    if (iter != g_unique_id_set.end()) {
        g_unique_id_set.erase(iter);
        return true;
    }
    return false;
}

int64_t add_timer(int interval, std::function<void()> func) {
    return time_pool.AddTimer(interval, func);
}

void rm_timer(int64_t timer_id) {
    time_pool.CancelTimer(timer_id);
}

bool loop() {
    time_pool.Update();
    bool is_busy = false;

#ifdef USE_ASYNC_REDIS
    if (async::redis::loop()) {
        is_busy = true;
    }
#endif

#ifdef USE_ASYNC_MONGO
    if (async::mongo::loop()) {
        is_busy = true;
    }
#endif

#ifdef USE_ASYNC_CURL
    if (async::curl::loop()) {
        is_busy = true;
    }
#endif

#ifdef USE_ASYNC_MYSQL
    if (async::mysql::loop()) {
        is_busy = true;
    }
#endif

    if (async::cpu::loop()) {
        is_busy = true;
    }

    return is_busy;
}

//////////////////////////////////////////////////////////////////////////

struct co_parallel_result {
    bool timeout_flag = false;
    int fns = 0;
};

int parallel(const std::initializer_list<fn_cb>& fns) {
    if (fns.size() == 0) {
        return E_CO_RETURN_ERROR;
    }

    unsigned int co_id = Coroutine::curid();
    if (co_id == M_MAIN_COROUTINE_ID) {
        assert(false);
        return E_CO_RETURN_ERROR;
    }

    co_parallel_result* result = new co_parallel_result;
    result->fns = fns.size();

    int64_t unique_id = co_async::gen_unique_id();
    int64_t timer_id = co_async::add_timer(10 * 1000, [result, co_id, unique_id]() {
        result->timeout_flag = true;
        co_async::rm_unique_id(unique_id);
        Coroutine::resume(co_id);
    });

    auto done = [unique_id, timer_id, co_id] () {
        if (!co_async::rm_unique_id(unique_id)) {
            return;
        }
        co_async::rm_timer(timer_id);
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

    co_async::add_unique_id(unique_id);
    Coroutine::yield();

    int ret = E_CO_RETURN_OK;
    if (result->timeout_flag) {
        ret = E_CO_RETURN_TIMEOUT;
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
        return E_CO_RETURN_ERROR;
    }

    unsigned int co_id = Coroutine::curid();
    if (co_id == M_MAIN_COROUTINE_ID) {
        assert(false);
        return E_CO_RETURN_ERROR;
    }

    co_series_result* result = new co_series_result;
    for (auto& f : fns) {
        result->fns.push_back(f);
    }

    int64_t unique_id = co_async::gen_unique_id();
    int64_t timer_id = co_async::add_timer(10 * 1000, [result, co_id, unique_id]() {
        result->timeout_flag = true;
        co_async::rm_unique_id(unique_id);
        Coroutine::resume(co_id);
    });

    auto done = [unique_id, timer_id, co_id] () {
        if (!co_async::rm_unique_id(unique_id)) {
            return;
        }
        co_async::rm_timer(timer_id);
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

    co_async::add_unique_id(unique_id);
    Coroutine::yield();

    int ret = E_CO_RETURN_OK;
    if (result->timeout_flag) {
        ret = E_CO_RETURN_TIMEOUT;
    }
    
    delete result;
    return ret;
}

}