/*----------------------------------------------------------------
// Copyright 2023
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

/*
 * 每个uri都会单独创建一条或多条zhandle_t连接作为连接池，数量取决于setThreadFunc函数并发的线程数。zhandle_t有自动重连接的机制，会一直重连下去
 * 每条zhandle_t可以同时处理多个请求，同一时刻只会有一个io线程拥有zhandle_t
*/

#pragma once

#include <functional>
#include <memory>
#include <string>
#include "common/async/zookeeper/zook_parser.h"
#include "common/async/zookeeper/zook_cmd.h"

namespace async {
namespace zookeeper {

typedef std::function<void(ZookParserPtr)> async_zookeeper_cb;

// uri: host[ip1:port1,ip2:port2]|scheme[user:pwd]
void execute(const std::string& uri, std::shared_ptr<BaseZookCmd> cmd, async_zookeeper_cb cb);

bool loop(uint32_t curTime);

void setThreadFunc(std::function<void(std::function<void()>)>);

// 设置zook内部的日志等级
void zooSetDebugLevel(uint32_t level);

}
}