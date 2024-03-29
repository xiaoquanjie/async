/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

//#define USE_ASYNC_MYSQL
#ifdef USE_ASYNC_MYSQL

#include "async/co_async/mysql/co_mysql.h"
#include "async/co_async/promise.h"
#include <mysql/mysql.h>


namespace co_async {
namespace mysql {

std::pair<int, async::mysql::MysqlReplyParserPtr> query(const std::string& uri, const std::string& sql, const TimeOut& t) {
    auto res = co_async::promise([&uri, &sql](co_async::Resolve resolve, co_async::Reject reject) {
        async::mysql::query(uri, sql, [resolve](async::mysql::MysqlReplyParserPtr ptr) {
            resolve(ptr);
        });
    }, t());

    std::pair<int, async::mysql::MysqlReplyParserPtr> ret = std::make_pair(res.first, nullptr);
    if (co_async::checkOk(res)) {
        ret.second = co_async::getOk<async::mysql::MysqlReplyParser>(res);
    }

    return ret;
}

std::pair<int, async::mysql::MysqlReplyParserPtr> execute(const std::string& uri, const std::string& sql, const TimeOut& t) {
    auto res = co_async::promise([&uri, &sql](co_async::Resolve resolve, co_async::Reject reject) {
        async::mysql::execute(uri, sql, [resolve](async::mysql::MysqlReplyParserPtr ptr) {
            resolve(ptr);
        });
    }, t());

    std::pair<int, async::mysql::MysqlReplyParserPtr> ret = std::make_pair(res.first, nullptr);
    if (co_async::checkOk(res)) {
        ret.second = co_async::getOk<async::mysql::MysqlReplyParser>(res);
    }

    return ret;
}

std::pair<int, int> execute(const std::string& uri, const std::string& sql, int& effectRow, const TimeOut& t) {
    auto res = execute(uri, sql, t);
    std::pair<int, int> ret = std::make_pair(res.first, 0);

    if (co_async::checkOk(res)) {
        auto parser = res.second;
        ret.second = parser->GetError();
        effectRow = parser->GetAffectedRow();
    }

    return ret;
}

std::pair<int, int> query(const std::string& uri, const std::string& sql, async::mysql::async_mysql_query_cb cb, const TimeOut& t) {
    auto res = query(uri, sql, t);
    std::pair<int, int> ret = std::make_pair(res.first, 0);

    if (co_async::checkOk(res)) {
        auto parser = res.second;
        ret.second = parser->GetError();

        int idx = 1;
        int affectedRow = parser->GetNumRow();
        int fields = parser->GetField();

        while (true) {
            auto row = parser->Next();
            if (row) {
                cb(ret.second, row, idx, fields, affectedRow);
            }
            else {
                break;
            }
        }
    }

    return ret;
}



}
}

#endif