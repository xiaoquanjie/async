/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

/*
 * 每个io线程都维护了自己的连接池
*/

#pragma once

#include <functional>
#include "async/async/mongo/mongo_cmd.h"
#include "async/async/mongo/mongo_reply_parser.h"

namespace async {
namespace mongo {

typedef std::function<void(MongoReplyParserPtr)> async_mongo_cb;

// @uri: addr|db|collection
void execute(std::string uri, const BaseMongoCmd& cmd, async_mongo_cb cb);

bool loop(uint32_t curTime);

void setThreadFunc(std::function<void(std::function<void()>)>);

}
}