/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include "common/async/mongo/async_mongo.h"

namespace co_async {
namespace mongo {

// pair.first表示是[E_OK/E_TIMEOUT]
// 详细信息放在返回值里
std::pair<int, async::mongo::MongoReplyParserPtr> execute(const std::string &uri, const async::mongo::BaseMongoCmd &cmd, int timeOut = 30 * 100);

// @返回值pair.second表示mongo语句操作是否成功
// @jsonResult表示返回的结果集
std::pair<int, bool> execute(const std::string &uri, const async::mongo::BaseMongoCmd &cmd, std::string* jsonResult, int timeOut = 30 * 100);

// @deletedCount是删除命令下的删除条数
std::pair<int, bool> execute(const std::string &uri, const async::mongo::BaseMongoCmd &cmd, int* deletedCount, int timeOut = 30 * 100);

// @modifiedCount, @matchedCount, @upsertedCount是更新命令下的返回
std::pair<int, bool> execute(const std::string &uri, const async::mongo::BaseMongoCmd &cmd, int* modifiedCount, int* matchedCount, int* upsertedCount, int timeOut = 30 * 100);

}
}