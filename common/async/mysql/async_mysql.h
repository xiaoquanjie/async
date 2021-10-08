/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include <functional>

namespace async {
namespace mysql {

// @int errno, MYSQL_ROW row, int row_idx, int fields, int affected_rows
typedef std::function<void(int, void*, int, int, int)> async_mysql_query_cb;

// @int errno, int affected_rows
typedef std::function<void(int, int)> async_mysql_exec_cb;

// 不支持查询多结果集，如果执行了返回多结果集的sql语句，行为是未可知的
void execute(const std::string& uri, const std::string& sql, async_mysql_query_cb cb);

void execute(const std::string& uri, const std::string& sql, async_mysql_exec_cb cb);

void set_max_connection(unsigned int cnt);

void set_keep_connection(unsigned int cnt);

bool loop();

void set_thread_func(std::function<void(std::function<void()>)>);

}    
}