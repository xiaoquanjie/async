/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include "common/async/mongo/async_mongo.h"
#include "common/co_async/co_io.h"

namespace co_async {
namespace mongo {

int get_wait_time();

void set_wait_time(int wait_time);

int execute(const std::string& uri, const async::mongo::BaseMongoCmd& cmd, async::mongo::async_mongo_cb cb);

int execute(const std::string& uri, const async::mongo::BaseMongoCmd& cmd);

int execute(const std::string& uri, const async::mongo::BaseMongoCmd& cmd, std::string& json_result);

int execute(const std::string& uri, const async::mongo::BaseMongoCmd& cmd, int& modifiedOrInsertCount);

int execute(const std::string& uri, const async::mongo::BaseMongoCmd& cmd, int& modifiedCount, int& upsertedCount);

}
}