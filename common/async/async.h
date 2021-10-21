/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include <functional>
#include <initializer_list>
#include "common/async/redis/async_redis.h"
#include "common/async/mongo/async_mongo.h"
#include "common/async/curl/async_curl.h"
#include "common/async/cpu/async_cpu.h"
#include "common/async/mysql/async_mysql.h"

namespace async {

bool loop();

// 添加入主循环中
void addToLoop(std::function<bool()> f);

///////////////////////////////////////////////////////////////////////////////////////////

typedef std::function<void(int err)> done_cb;
typedef std::function<void(int err)> next_cb;
typedef std::function<void(next_cb)> fn_cb;

void parallel(done_cb done, const std::initializer_list<fn_cb>& fns);

void series(done_cb done, const std::initializer_list<fn_cb>& fns);

void split(const std::string source, const std::string &separator, std::vector<std::string> &array);

/*
*example

async::parallel([](int err) {
    std::cout << "done:" << err << std::endl;
},
{
    [](async::next_cb next) {
        async::curl::get("http://baidu.com", [next](int, int, std::string& body) {
            std::cout << "body1:" << body.length() <<std::endl;
            next(0);
        });
    },
    [](async::next_cb next) {
        async::curl::get("http://baidu.com", [next](int, int, std::string& body) {
            std::cout << "body2:" << body.length() <<std::endl;
            next(2);
        });
    }
});

*/

};