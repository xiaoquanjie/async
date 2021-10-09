/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

//#define USE_ASYNC_MYSQL
#ifdef USE_ASYNC_MYSQL

#include "common/co_async/co_mysql.h"
#include "common/coroutine/coroutine.hpp"
#include <mariadb/mysql.h>

namespace co_async {
namespace mysql {

int g_wait_time = 5 * 1000;

int get_wait_time() {
    return g_wait_time;
}

void set_wait_time(int wait_time) {
    g_wait_time = wait_time;
}

struct co_mysql_result {
    bool timeout_flag = false;
    int err = 0;
    int affected_row = 0;
    MYSQL_RES* res = 0;
};

int execute(const std::string& uri, const std::string& sql, async::mysql::async_mysql_query_cb cb) {
    unsigned int co_id = Coroutine::curid();
    if (co_id == M_MAIN_COROUTINE_ID) {
        assert(false);
        return E_CO_RETURN_ERROR;
    }

    int64_t unique_id = co_async::gen_unique_id();
    co_mysql_result* result = new co_mysql_result;

    int64_t timer_id = co_async::add_timer(g_wait_time, [result, co_id, unique_id]() {
        result->timeout_flag = true;
        co_async::rm_unique_id(unique_id);
        Coroutine::resume(co_id);
    });

    async::mysql::execute(uri, sql, [result, timer_id, co_id, unique_id](int err, void* res) {
        if (!co_async::rm_unique_id(unique_id)) {
            mysql_free_result((MYSQL_RES*)res);
            return;
        }
        co_async::rm_timer(timer_id);
        result->res = (MYSQL_RES*)res;
        result->err = err;
        Coroutine::resume(co_id);
    });

    co_async::add_unique_id(unique_id);
    Coroutine::yield();

    int ret = E_CO_RETURN_OK;
    if (result->timeout_flag) {
        ret = E_CO_RETURN_TIMEOUT;
    }
    else {
        int affected_rows = 0;
        int fields_cnt = 0;
        if (result->res && result->err == 0) {
            affected_rows = (int)mysql_num_rows(result->res);
            fields_cnt = (int)mysql_num_fields(result->res);
        }
        if (affected_rows == 0) {
            cb(result->err, 0, 0, fields_cnt, affected_rows);
        }
        else {
            // 有结果
            MYSQL_ROW row = 0;
            int idx = 1;
            while (0 != (row = mysql_fetch_row(result->res))) {
                cb(result->err, (void*)row, idx, fields_cnt, affected_rows);
                ++idx;
            }
        }
    }
    
    if (result->res) {
        mysql_free_result(result->res);
    }
    delete result;
    return ret;
}

int execute(const std::string& uri, const std::string& sql, async::mysql::async_mysql_exec_cb cb) {
    unsigned int co_id = Coroutine::curid();
    if (co_id == M_MAIN_COROUTINE_ID) {
        assert(false);
        return E_CO_RETURN_ERROR;
    }

    int64_t unique_id = co_async::gen_unique_id();
    co_mysql_result* result = new co_mysql_result;

    int64_t timer_id = co_async::add_timer(g_wait_time, [result, co_id, unique_id]() {
        result->timeout_flag = true;
        co_async::rm_unique_id(unique_id);
        Coroutine::resume(co_id);
    });

    async::mysql::execute(uri, sql, [result, timer_id, co_id, unique_id](int err, int affected_row) {
        if (!co_async::rm_unique_id(unique_id)) {
            return;
        }
        co_async::rm_timer(timer_id);
        result->affected_row = affected_row;
        result->err = err;
        Coroutine::resume(co_id);
    });

    co_async::add_unique_id(unique_id);
    Coroutine::yield();

    int ret = E_CO_RETURN_OK;
    if (result->timeout_flag) {
        ret = E_CO_RETURN_TIMEOUT;
    }
    else {
        cb(result->err, result->affected_row);
    }

    delete result;
    return ret;
}


}
}

#endif