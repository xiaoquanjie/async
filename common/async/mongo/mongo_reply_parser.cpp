/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_MONGO

#include "common/async/mongo/mongo_reply_parser.h"
#include <vector>
#include <bson.h>

namespace async {
namespace mongo {

MongoReplyParser::MongoReplyParser() {
    this->error = malloc(sizeof(bson_error_t));
    this->bsons = 0;
    this->bson_idx = 0;
    this->bsons = new std::vector<void*>();
}

MongoReplyParser::~MongoReplyParser() {
    free(this->error);
    this->error = 0;
    if (this->bsons) {
        auto bson_vec = (std::vector<void*>*)(this->bsons);
        for (auto p : *bson_vec) {
            bson_destroy((bson_t*)p);
        }
        delete bson_vec;
        this->bsons = 0;
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
    if (!this->bsons) {
        return 0;
    }

    auto bson_vec = (std::vector<void*>*)(this->bsons);
    if (this->bson_idx >= bson_vec->size()) {
        return 0;
    }

    return (*bson_vec)[this->bson_idx++];
}

void MongoReplyParser::ResetNextBson() {
    this->bson_idx = 0;
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