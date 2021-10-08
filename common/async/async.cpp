/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#include "common/async/async.h"
#include <vector>

namespace async {

bool loop() {
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

/////////////////////////////////////////////////

struct parallel_result {
    int fns = 0;
    int err = 0;
};

void parallel(done_cb done, const std::initializer_list<fn_cb>& fns) {
    if (fns.size() == 0) {
        return;
    }

    parallel_result* result = new parallel_result;
    result->fns = fns.size();
    
    next_cb next = [result, done](int err) {
        if (err != 0) {
            result->err |= err;
        }
        result->fns -= 1;
        if (result->fns == 0) {
            int e = result->err;
            delete result;
            done(e);
        }
    };

    for (auto& f : fns) {
        f(next);
    }
}

///////////////////////////////////////////////////////////////////
struct series_result {
    next_cb next;
    std::vector<fn_cb> fns;
    size_t idx = 0;
    int err = 0;
};

void series(done_cb done, const std::initializer_list<fn_cb>& fns) {
    if (fns.size() == 0) {
        return;
    }

    series_result* result = new series_result;
    for (auto& f : fns) {
        result->fns.push_back(f);
    }

    next_cb next = [result, done](int err) {
        if (err != 0) {
            result->err |= err;
        }
        result->idx++;
        if (result->idx == result->fns.size()) {
            int e = result->err;
            delete result;
            done(e);
        }    
        else {
            result->fns[result->idx](result->next);
        }
    };
    
    result->next = next;
    result->fns[result->idx](next);
}

///////////////////////////////////////////////////////////////////

// 字符串分割函数
void split(const std::string source, 
    const std::string &separator, 
    std::vector<std::string> &array) {
    array.clear();
    std::string::size_type start = 0;
    while (true) {
        std::string::size_type pos = source.find_first_of(separator, start);
        if (pos == std::string::npos) {
            std::string sub = source.substr(start, source.size());
            array.push_back(sub);
            break;
        }

        std::string sub = source.substr(start, pos - start);
        start = pos + separator.size();
        array.push_back(sub);
    }
}

}