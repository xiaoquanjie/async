/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include <memory>
#include <functional>
#include <vector>

namespace async {

namespace mongo {

// 如果是返回的结果集，必须先调用完NextJson才知道IsOk()函数的正确性
struct MongoReplyParser {
    MongoReplyParser();

    ~MongoReplyParser();

    bool IsOk();

    uint32_t Code();

    const char* What();

    // 返回一个bson_t指针
    const void* NextBson();

    char* NextJson();

    void FreeJson(void* json);

    void ResetNextBson();

    bool GetDeletedCount(int& cnt);

    bool GetModifiedCount(int& cnt);

    bool GetKeyValue(const char* key, int& v);

    bool GetMatchedCount(int& cnt);

    bool GetUpsertedCount(int& cnt);

    int op_result = -1;  // 0表示成功
    void* error;
    void* cursor;
    std::vector<void*> bsons;
    size_t bson_idx;
};

typedef std::shared_ptr<MongoReplyParser> MongoReplyParserPtr;

} // mongo
} // async
