/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include <memory>
#include <functional>
#include <vector>

namespace async {

namespace mongo {

// 如果是返回的结果集，必须先调用完NextCursor才知道IsOk()函数的正确性
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

    int op_result = -1;  // 0表示成功
    void* error;
    void* cursor;
    std::vector<void*> bsons;
    size_t bson_idx;
};

typedef std::shared_ptr<MongoReplyParser> MongoReplyParserPtr;

} // mongo
} // async
