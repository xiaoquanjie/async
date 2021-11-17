/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

/*
 * 每个io线程都维护了自己的连接池
*/

#pragma once

#include <functional>
#include "common/async/mongo/mongo_cmd.h"
#include "common/async/mongo/mongo_reply_parser.h"

namespace async {
namespace mongo {

typedef std::function<void(MongoReplyParserPtr)> async_mongo_cb;

// @uri: [mongourl-db-collection]
void execute(std::string uri, const BaseMongoCmd& cmd, async_mongo_cb cb);

bool loop();

void setThreadFunc(std::function<void(std::function<void()>)>);

// 设置日志接口, 要求是线程安全的
void setLogFunc(std::function<void(const char*)> cb);

void log(const char* format, ...);

}
}