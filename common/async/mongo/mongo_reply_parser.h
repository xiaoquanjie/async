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

// ����Ƿ��صĽ�����������ȵ�����NextJson��֪��IsOk()��������ȷ��
struct MongoReplyParser {
    MongoReplyParser();

    ~MongoReplyParser();

    bool IsOk();

    uint32_t Code();

    const char* What();

    // ����һ��bson_tָ��
    const void* NextBson();

    char* NextJson();

    void FreeJson(void* json);

    void ResetNextBson();

    bool GetDeletedCount(int& cnt);

    bool GetModifiedCount(int& cnt);

    bool GetKeyValue(const char* key, int& v);

    bool GetMatchedCount(int& cnt);

    bool GetUpsertedCount(int& cnt);

    int op_result = -1;  // 0��ʾ�ɹ�
    void* error;
    void* cursor;
    std::vector<void*> bsons;
    size_t bson_idx;
};

typedef std::shared_ptr<MongoReplyParser> MongoReplyParserPtr;

} // mongo
} // async
