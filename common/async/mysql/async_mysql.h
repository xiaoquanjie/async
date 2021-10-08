#pragma once

#include <functional>

namespace async {
namespace mysql {

// @int errno, MYSQL_ROW row, int fields, int row_idx, int affected_rows
typedef std::function<void(int, void*, int, int, int)> async_mysql_query_cb;

typedef std::function<void(int, int)> async_mysql_exec_cb;

void execute(const std::string& uri, const std::string& sql, async_mysql_query_cb cb);

void execute(const std::string& uri, const std::string& sql, async_mysql_exec_cb cb);

bool loop();

void set_thread_func(std::function<void(std::function<void()>)>);

}    
}