/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include "common/async/mysql/async_mysql.h"
#include "common/co_async/co_io.h"

namespace co_async {
namespace mysql {

int getWaitTime();

void setWaitTime(int wait_time);

// 不支持查询多结果集，如果执行了返回多结果集的sql语句，行为是未可知的
int execute(const std::string& uri, const std::string& sql, async::mysql::async_mysql_query_cb cb);

int execute(const std::string& uri, const std::string& sql, async::mysql::async_mysql_exec_cb cb);


}
}