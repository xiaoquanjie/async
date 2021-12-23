/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_MONGO

#include "common/async/mongo/mongo_cmd.h"
#include <mongoc.h>
#include <bson.h>

namespace async {
namespace mongo {

BaseMongoCmd::Data::~Data()
{
    if (this->doc_bson_ptr)
    {
        destory_bson((bson_t *)this->doc_bson_ptr);
    }
    if (this->update_bson_ptr)
    {
        destory_bson((bson_t *)this->update_bson_ptr);
    }
    if (this->opt_bson_ptr)
    {
        destory_bson((bson_t *)this->opt_bson_ptr);
    }
}

BaseMongoCmd::BaseMongoCmd() {
}

BaseMongoCmd::~BaseMongoCmd() {
}

bool BaseMongoCmd::IsDeleteCmd() const {
    if (!this->d) {
        return false;
    }
    return this->d->cmd == "deletemany" || this->d->cmd == "deleteone";
}

bool BaseMongoCmd::IsUpdateCmd() const {
    if (!this->d) {
        return false;
    }
    return this->d->cmd == "updatemany" || this->d->cmd == "updateone";
}

bool BaseMongoCmd::IsInsertCmd() const {
    if (!this->d) {
        return false;
    }
    return this->d->cmd == "insert";
}

std::string BaseMongoCmd::DebugString() {
    if (!this->d) {
        return "";
    }

    std::string str;
    if (this->d->doc_bson_ptr) {
        char* s = bson_as_canonical_extended_json((bson_t*)this->d->doc_bson_ptr, NULL);
        str += std::string("doc_bson:") + s;
        bson_free(s);
    }
    if (this->d->update_bson_ptr) {
        str += ",";
        char* s = bson_as_canonical_extended_json((bson_t*)this->d->update_bson_ptr, NULL);
        str += std::string("update_bson:") + s;
        bson_free(s);
    }
    if (this->d->opt_bson_ptr) {
        str += ",";
        char* s = bson_as_canonical_extended_json((bson_t*)this->d->opt_bson_ptr, NULL);
        str += std::string("opt_bson:") + s;
        bson_free(s);
    }

    return str;
}

///////////////////////////////////////////////////////////////

InsertMongoCmd::InsertMongoCmd(const std::string& json) {
    this->d = std::make_shared<Data>();
    this->d->cmd = "insert";
    this->d->doc_bson_ptr = new_from_json(json);
}

InsertMongoCmd::InsertMongoCmd(const std::initializer_list<MongoKeyValue> &fields)
{
    this->d = std::make_shared<Data>();
    this->d->cmd = "insert";
    this->d->doc_bson_ptr = new_from_json("");
    bson_append(this->d->doc_bson_ptr, fields);
}

///////////////////////////////////////////////////////////////

FindMongoCmd::FindMongoCmd(const std::string& query, bool is_idx) {
    this->d = std::make_shared<Data>();
    this->d->cmd = "find";
    this->d->doc_bson_ptr = new_from_query(query, is_idx);
}

FindMongoCmd::FindMongoCmd(const std::initializer_list<MongoKeyValueCmp> &query_fields,
                           const std::initializer_list<std::string> &include_fields,
                           const std::initializer_list<std::string> &except_fields)
{
    this->d = std::make_shared<Data>();
    this->d->cmd = "find";
    this->d->doc_bson_ptr = new_from_json("");
    bson_append(this->d->doc_bson_ptr, query_fields);
    if (include_fields.size()) {
        this->d->opt_bson_ptr = new_from_json("");
        bson_append(this->d->opt_bson_ptr, include_fields, true);
    }
    else if (except_fields.size()) {
        this->d->opt_bson_ptr = new_from_json("");
        bson_append(this->d->opt_bson_ptr, except_fields, false);
    }
}

FindMongoCmd::FindMongoCmd(const std::initializer_list<MongoKeyValueCmp> &query_fields,
                           const std::initializer_list<MongoKeyValue> &sort_fiels,
                           uint32_t limit)
{
    this->d = std::make_shared<Data>();
    this->d->cmd = "find_opts";
    this->d->doc_bson_ptr = new_from_json("");
    bson_append(this->d->doc_bson_ptr, query_fields);

    if (limit > 0 || sort_fiels.size()) {
        this->d->opt_bson_ptr = new_from_json("");
    }
    if (limit > 0) {
        bson_append(this->d->opt_bson_ptr, "limit", limit);
    }
    if (sort_fiels.size()) {
        bson_append(this->d->opt_bson_ptr, "sort", sort_fiels);
    }
}

///////////////////////////////////////////////////////////////

DeleteOneMongoCmd::DeleteOneMongoCmd(const std::string& query, bool is_idx) {
    this->d = std::make_shared<Data>();
    this->d->cmd = "deleteone";
    this->d->doc_bson_ptr = new_from_query(query, is_idx);
}

DeleteOneMongoCmd::DeleteOneMongoCmd(const std::initializer_list<MongoKeyValueCmp> &option_fields)
{
    this->d = std::make_shared<Data>();
    this->d->cmd = "deleteone";
    this->d->doc_bson_ptr = new_from_json("");
    bson_append(this->d->doc_bson_ptr, option_fields);
}

///////////////////////////////////////////////////////////////

DeleteManyMongoCmd::DeleteManyMongoCmd(const std::string& query, bool is_idx) {
    this->d = std::make_shared<Data>();
    this->d->cmd = "deletemany";
    this->d->doc_bson_ptr = new_from_query(query, is_idx);
}

DeleteManyMongoCmd::DeleteManyMongoCmd(const std::initializer_list<MongoKeyValueCmp> &option_fields)
{
    this->d = std::make_shared<Data>();
    this->d->cmd = "deletemany";
    this->d->doc_bson_ptr = new_from_json("");
    bson_append(this->d->doc_bson_ptr, option_fields);
}

///////////////////////////////////////////////////////////////

UpdateOneMongoCmd::UpdateOneMongoCmd(const std::initializer_list<MongoKeyValueCmp> &option_fields,
                                     const std::initializer_list<MongoKeyValue> &update_fields,
                                     bool upsert) {
    this->d = std::make_shared<Data>();
    this->d->cmd = "updateone";
    this->d->doc_bson_ptr = new_from_json("");
    bson_append(this->d->doc_bson_ptr, option_fields); 
    
    this->d->update_bson_ptr = new_from_json("");
    bson_append(this->d->update_bson_ptr, "$set", update_fields);
    
    this->d->opt_bson_ptr = new_from_json("");
    bson_append(this->d->opt_bson_ptr, "upsert", upsert);
}

///////////////////////////////////////////////////////////////

UpdateManyMongoCmd::UpdateManyMongoCmd(const std::initializer_list<MongoKeyValueCmp> &option_fields,
                                       const std::initializer_list<MongoKeyValue> &update_fields,
                                       bool upsert)
{
    this->d = std::make_shared<Data>();
    this->d->cmd = "updatemany";
    this->d->doc_bson_ptr = new_from_json("");
    bson_append(this->d->doc_bson_ptr, option_fields); 
    
    this->d->update_bson_ptr = new_from_json("");
    bson_append(this->d->update_bson_ptr, "$set", update_fields);
    
    this->d->opt_bson_ptr = new_from_json("");
    bson_append(this->d->opt_bson_ptr, "upsert", upsert);
}

///////////////////////////////////////////////////////////////

CreateIndexMongoCmd::CreateIndexMongoCmd(const std::vector<std::string> &fields) {
    this->d = std::make_shared<Data>();
    this->d->cmd = "createidx";
    this->d->doc_bson_ptr = new_from_json("");
    //bson_init((bson_t*)this->d->doc_bson_ptr);
    for (auto& item: fields) {
        bson_append(this->d->doc_bson_ptr, item.c_str(), 1);
    }
}

CreateIndexMongoCmd::CreateIndexMongoCmd(const std::initializer_list<std::string> &fields) {
    this->d = std::make_shared<Data>();
    this->d->cmd = "createidx";
    this->d->doc_bson_ptr = new_from_json("");
    //bson_init((bson_t*)this->d->doc_bson_ptr);
    for (auto& item: fields) {
        bson_append(this->d->doc_bson_ptr, item.c_str(), 1);
    }
}

CreateExpireIndexMongoCmd::CreateExpireIndexMongoCmd() {
    this->d = std::make_shared<Data>();
    this->d->cmd = "createexpireidx";
    this->d->doc_bson_ptr = new_from_json("");
}

} // mongo
} // async

#endif