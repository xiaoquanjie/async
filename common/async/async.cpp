/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#include "common/async/async.h"
#include <vector>

namespace async {

struct funcItem {
    std::function<bool(uint32_t)> loopFunc;
    std::function<void(std::function<void(const char*)>)> logFunc;
    std::function<void(std::function<void(std::function<void()>)>)> thrFunc;
};

std::vector<funcItem> g_func_item_array = {
#ifdef USE_ASYNC_REDIS
    {
        std::bind(async::redis::loop, std::placeholders::_1),
        std::bind(async::redis::setLogFunc, std::placeholders::_1),
        std::bind(async::redis::setThreadFunc, std::placeholders::_1)
    },
#endif

#ifdef USE_ASYNC_MONGO
    {
        std::bind(async::mongo::loop, std::placeholders::_1),
        std::bind(async::mongo::setLogFunc, std::placeholders::_1),
        std::bind(async::mongo::setThreadFunc, std::placeholders::_1)
    },
#endif

#ifdef USE_ASYNC_CURL
    {
        std::bind(async::curl::loop, std::placeholders::_1),
        std::bind(async::curl::setLogFunc, std::placeholders::_1),
        std::bind(async::curl::setThreadFunc, std::placeholders::_1)
    },
#endif

#ifdef USE_ASYNC_MYSQL
    {
        std::bind(async::mysql::loop, std::placeholders::_1),
        std::bind(async::mysql::setLogFunc, std::placeholders::_1),
        std::bind(async::mysql::setThreadFunc, std::placeholders::_1)
    },
#endif

    {
        std::bind(async::cpu::loop, std::placeholders::_1),
        std::bind(async::cpu::setLogFunc, std::placeholders::_1),
        std::bind(async::cpu::setThreadFunc, std::placeholders::_1)
    }
};

////////////////////////////////////////////////////////////////////////////////////

bool loop(uint32_t cur_time) {
    if (cur_time == 0) {
       // time(&cur_time);
    }

    bool is_busy = false;
    for (auto& item : g_func_item_array) {
        if (item.loopFunc) {
            if (item.loopFunc(cur_time)) {
                is_busy = true;
            }
        }
    }
    
    return is_busy;
}

void addToLoop(std::function<bool(uint32_t)> f) {
   g_func_item_array.push_back({f, nullptr, nullptr});
}

void setLogFunc(std::function<void(const char*)> cb) {
    for (auto& item : g_func_item_array) {
        if (item.logFunc) {
            item.logFunc(cb);
        }
    }
}

void setThreadFunc(std::function<void(std::function<void()>)> cb) {
    for (auto& item : g_func_item_array) {
        if (item.thrFunc) {
            item.thrFunc(cb);
        }
    }
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