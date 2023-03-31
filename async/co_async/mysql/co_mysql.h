/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include "async/async/mysql/async_mysql.h"
#include "async/co_async/comm.hpp"

namespace co_async {
namespace mysql {

// 不支持查询多结果集，如果执行了返回多结果集的sql语句，行为是未可知的
std::pair<int, async::mysql::MysqlReplyParserPtr> query(const std::string& uri, const std::string& sql, const TimeOut& t = TimeOut()); 

std::pair<int, async::mysql::MysqlReplyParserPtr> execute(const std::string& uri, const std::string& sql, const TimeOut& t = TimeOut()); 

// 不支持查询多结果集，如果执行了返回多结果集的sql语句，行为是未可知的
std::pair<int, int> query(const std::string& uri, const std::string& sql, async::mysql::async_mysql_query_cb cb, const TimeOut& t = TimeOut());

// pair.second表示错误码
std::pair<int, int> execute(const std::string& uri, const std::string& sql, int& effectRow, const TimeOut& t = TimeOut());

}
}