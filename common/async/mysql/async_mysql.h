/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include <functional>
#include "common/async/mysql/mysql_reply_parser.h"

namespace async {
namespace mysql {

// @int errno, MYSQL_ROW row, int row_idx, int fields, int affected_rows
typedef std::function<void(int, const void*, int, int, int)> async_mysql_query_cb;

// @int errno, int affected_rows
typedef std::function<void(int, int)> async_mysql_exec_cb;

// @int errno, MYSQL_RES*
typedef std::function<void(int, const void*)> async_mysql_query_cb2;

typedef  std::function<void(MysqlReplyParserPtr)> async_mysql_cb;

// 不支持查询多结果集，如果执行了返回多结果集的sql语句，行为是未可知的
void query(const std::string& id, const std::string& sql, async_mysql_cb cb);

void execute(const std::string& id, const std::string& sql, async_mysql_cb cb);

///////////////

void setMaxConnection(uint32_t cnt);

bool loop(uint32_t curTime);

void setThreadFunc(std::function<void(std::function<void()>)>);

}    
}