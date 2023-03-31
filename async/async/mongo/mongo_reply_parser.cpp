/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_MONGO

#include "async/async/mongo/mongo_reply_parser.h"
#include <bson.h>

namespace async {
namespace mongo {

MongoReplyParser::MongoReplyParser() {
    this->error = malloc(sizeof(bson_error_t));
    this->bson_idx = 0;
}

MongoReplyParser::~MongoReplyParser() {
    free(this->error);
    this->error = 0;
    for (auto p : this->bsons) {
        bson_destroy((bson_t*)p);
    }
}

bool MongoReplyParser::IsOk() {
    return this->op_result == 0;
}

uint32_t MongoReplyParser::Code() {
    return ((bson_error_t*)this->error)->code;
}

const char* MongoReplyParser::What() {
    return ((bson_error_t*)this->error)->message;
}

const void* MongoReplyParser::NextBson() {
    if (this->bson_idx >= this->bsons.size()) {
        return 0;
    }

    return this->bsons[this->bson_idx++];
}

void MongoReplyParser::ResetNextBson() {
    this->bson_idx = 0;
}

bool MongoReplyParser::GetDeletedCount(int& cnt) {
    return GetKeyValue("deletedCount", cnt);
}

bool MongoReplyParser::GetModifiedCount(int& cnt) {
    return GetKeyValue("modifiedCount", cnt);
}

bool MongoReplyParser::GetMatchedCount(int& cnt) {
    return GetKeyValue("matchedCount", cnt);
}

bool MongoReplyParser::GetUpsertedCount(int& cnt) {
    return GetKeyValue("upsertedCount", cnt);
}

bool MongoReplyParser::GetKeyValue(const char* key, int& v) {
    if (this->bsons.empty()) {
        return false;
    }

    bson_t* b = (bson_t*)this->bsons[0];
    bson_iter_t iter;
    if (!bson_iter_init(&iter, (const bson_t*)b)) {
        return false;
    }

    bson_iter_t desc;
    if (!bson_iter_find_descendant(&iter, key, &desc)) {
        return false;
    }

    v = (int)bson_iter_as_int64(&desc);
    return true;
}

char* MongoReplyParser::NextJson() {
    const bson_t* doc = (const bson_t*)this->NextBson();
    if (doc) {
        char* json = bson_as_relaxed_extended_json(doc, nullptr);
        return json;
    }
    return 0;
}

void MongoReplyParser::FreeJson(void* json) {
    bson_free(json);
}

} // mongo
} // async

#endif