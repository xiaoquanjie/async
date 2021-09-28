/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

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

void set_thread_func(std::function<void(std::function<void()>)>);

}
}