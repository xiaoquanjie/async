/*----------------------------------------------------------------
// Copyright 2023
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include "async/async/zookeeper/async_zookeeper.h"
#include "async/co_async/comm.hpp"

namespace co_async {
namespace zookeeper {

// pair.first表示协程是否成功
std::pair<int, async::zookeeper::ZookParserPtr> execute(const std::string& uri, std::shared_ptr<async::zookeeper::BaseZookCmd> cmd, const TimeOut& t = TimeOut());

int execute(const std::string& uri, std::shared_ptr<async::zookeeper::BaseZookCmd> cmd, bool& ok, const TimeOut& t = TimeOut());

// pair.second表示命令执行是否成功
std::pair<int, bool> execute2(const std::string& uri, std::shared_ptr<async::zookeeper::BaseZookCmd> cmd, const TimeOut& t = TimeOut());

}
}