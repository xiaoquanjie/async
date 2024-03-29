/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include "async/async/mongo/async_mongo.h"
#include "async/co_async/comm.hpp"

namespace co_async {
namespace mongo {

// pair.first表示是[E_OK/E_TIMEOUT]
// 详细信息放在返回值里
std::pair<int, async::mongo::MongoReplyParserPtr> execute(const std::string &uri, const async::mongo::BaseMongoCmd &cmd, const TimeOut& t = TimeOut());

// @返回值pair.second表示mongo语句操作是否成功
// @jsonResult表示返回的结果集
std::pair<int, bool> execute(const std::string &uri, const async::mongo::BaseMongoCmd &cmd, std::string* jsonResult, const TimeOut& t = TimeOut());

// @deletedCount是删除命令下的删除条数
std::pair<int, bool> execute(const std::string &uri, const async::mongo::BaseMongoCmd &cmd, int* deletedCount, const TimeOut& t = TimeOut());

// @modifiedCount, @matchedCount, @upsertedCount是更新命令下的返回
std::pair<int, bool> execute(const std::string &uri, const async::mongo::BaseMongoCmd &cmd, int* modifiedCount, int* matchedCount, int* upsertedCount, const TimeOut& t = TimeOut());

}
}