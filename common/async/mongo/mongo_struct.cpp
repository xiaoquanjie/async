/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#include "common/async/mongo/mongo_struct.h"
#include <mongoc.h>
#include <bson.h>

namespace async {

namespace mongo {

void destory_bson(void* p) {
    if (p) {
        bson_destroy((bson_t*)p);
    }
}

/////////////////////////////////////////////////////////////////

void* new_from_json(const std::string& json) {
    if (json.empty()) {
        return bson_new();
    }
    else {
        bson_error_t error;
        auto p = bson_new_from_json((const uint8_t *)json.c_str(), -1, &error);
        return p;
    }
}

void* new_from_query(const std::string& query, bool is_idx) {
    if (is_idx) {
        void* b = new_from_json("");
        bson_append_idx(b, query);
        return b;
    }
    else {
        return new_from_json(query);
    }
}

void bson_append_idx(void *b, const std::string &idx)
{
    bson_oid_t oid;
    bson_oid_init_from_data(&oid, (const uint8_t *)idx.c_str());
    BSON_APPEND_OID((bson_t *)b, "_id", &oid);
}

void bson_append(void *b, const char *field, const std::string& val) {
    bson_append(b, field, val.c_str());
}

void bson_append(void* b, const char* field, const char* val) {
    BSON_APPEND_UTF8((bson_t*)b, field, val);
}

void bson_append(void* b, const char* field, int32_t val) {
    BSON_APPEND_INT32((bson_t*)b, field, val);
}

void bson_append(void* b, const char* field, uint32_t val) {
    BSON_APPEND_INT32((bson_t*)b, field, val);
}

void bson_append(void* b, const char* field, int64_t val) {
    BSON_APPEND_INT64((bson_t*)b, field, val);
}

void bson_append(void* b, const char* field, uint64_t val) {
    BSON_APPEND_INT64((bson_t*)b, field, val);
}

void bson_append(void* b, const char* field, double val) {
    BSON_APPEND_DOUBLE((bson_t*)b, field, val);
}

void bson_append(void* b, const char* field, void* val) {
    BSON_APPEND_DOCUMENT((bson_t*)b, field, (bson_t*)val);
}

void bson_append(void* b, const char* field, bool val) {
    BSON_APPEND_BOOL((bson_t*)b, field, val);
}

void bson_append(void* b, const char* field, const char* val, size_t size) {
    BSON_APPEND_BINARY((bson_t*)b, field, BSON_SUBTYPE_BINARY, (const uint8_t*)val, size);
}

void bson_append(void* b, const char* field) {
    BSON_APPEND_NULL((bson_t*)b, field);
}

void bson_append(void* b, const char* cmp, const MongoKeyValue& field) {
    bson_t bson;
    BSON_APPEND_DOCUMENT_BEGIN((bson_t*)b, field.d->key.c_str(), (bson_t*)&bson);
    // 构造一个新的MongoKeyValue
    MongoKeyValue kv(cmp, 0);
    kv = field;
    kv.d->key = cmp;
    kv.d->cmp = "";
    bson_append(&bson, {kv});
    bson_append_document_end((bson_t*)b, &bson);
}

void bson_append(void* b,  const char* field, const std::initializer_list<MongoKeyValue> &fields) {
    bson_t bson;
    BSON_APPEND_DOCUMENT_BEGIN((bson_t*)b, field, (bson_t*)&bson);
    bson_append(&bson, fields);
    bson_append_document_end((bson_t*)b, &bson);
}

void bson_append(void* b, const std::initializer_list<MongoKeyValue> &fields) {
    for (const MongoKeyValue& item : fields) {
        if (item.is_int()) {
            bson_append(b, item.d->key.c_str(), item.d->meta.i);
            continue;
        }
        else if (item.is_uint()) {
            bson_append(b, item.d->key.c_str(), (uint32_t)item.d->meta.i);
            continue;
        }
        else if (item.is_double()) {
            bson_append(b, item.d->key.c_str(), item.d->meta.f);
            continue;
        }
        else if (item.is_str()) {
            bson_append(b, item.d->key.c_str(), (const char*)item.d->meta.str);
            continue;
        }
        else if (item.is_bson()) {
            bson_append(b, item.d->key.c_str(), (void*)item.d->meta.bson);
            continue;
        }
        else if (item.is_bool()) {
            bson_append(b, item.d->key.c_str(), (bool)item.d->meta.i);
            continue;
        }
        else if (item.is_null()) {
            bson_append(b, item.d->key.c_str());
            continue;
        }
        else if (item.is_int64()) {
            bson_append(b, item.d->key.c_str(), (int64_t)item.d->meta.i64);
            continue;
        }
        else if (item.is_uint64()) {
            bson_append(b, item.d->key.c_str(), (uint64_t)item.d->meta.i64);
            continue;
        }
        else if (item.is_binary()) {
            bson_append(b, item.d->key.c_str(), item.d->meta.str, item.d->str_len);
            continue;
        }
        else if (item.is_array()) {
            bson_array_append(b, item.d->key.c_str(), item.d->meta.bson);
            continue;
        }
    }
}

void bson_append(void* b, const std::initializer_list<MongoKeyValueCmp> &fields) {
    for (auto& item : fields) {
        if (item.is_int()) {
            if (item.has_cmp()) {
                bson_append(b, item.d->cmp.c_str(), item);
            }
            else {
                bson_append(b, item.d->key.c_str(), item.d->meta.i);
            }
            continue;
        }
        else if (item.is_uint()) {
            if (item.has_cmp()) {
                bson_append(b, item.d->cmp.c_str(), item);
            }
            else {
                bson_append(b, item.d->key.c_str(), (uint32_t)item.d->meta.i);
            }
            continue;
        }
        else if (item.is_double()) {
            if (item.has_cmp()) {
                bson_append(b, item.d->cmp.c_str(), item);
            }
            else {
                bson_append(b, item.d->key.c_str(), item.d->meta.f);
            }
            continue;
        }
        else if (item.is_str()) {
            bson_append(b, item.d->key.c_str(), (const char*)item.d->meta.str);
            continue;
        }
        else if (item.is_bson()) {
            bson_append(b, item.d->key.c_str(), (void*)item.d->meta.bson);
            continue;
        }
        else if (item.is_bool()) {
            bson_append(b, item.d->key.c_str(), (bool)item.d->meta.i);
            continue;
        }
        else if (item.is_null()) {
            bson_append(b, item.d->key.c_str());
            continue;
        }
        else if (item.is_int64()) {
            if (item.has_cmp()) {
                bson_append(b, item.d->cmp.c_str(), item);
            }
            else {
                bson_append(b, item.d->key.c_str(), (int64_t)item.d->meta.i64);   
            }
            continue;
        }
        else if (item.is_uint64()) {
            if (item.has_cmp()) {
                bson_append(b, item.d->cmp.c_str(), item);
            }
            else {
                bson_append(b, item.d->key.c_str(), (uint64_t)item.d->meta.i64);   
            }
            continue;
        }
        else if (item.is_array()) {
            if (item.has_cmp()) {
                bson_append(b, item.d->cmp.c_str(), item);
            }
            else {
                bson_array_append(b, item.d->key.c_str(), item.d->meta.bson);
            }
            continue;
        }
    }
}

void bson_append(void* b, const std::initializer_list<std::string>& fields, bool include) {
    for (auto& item : fields) {
        bson_append(b, item.c_str(), (int32_t)(include ? 1 : 0));
    }
}

void bson_array_append(void *b, const char *field, void* val) {
    BSON_APPEND_ARRAY((bson_t*)b, field, (bson_t*)val);
}

/////////////////////////////////////////////////////////////////////////////

MongoKeyValue::~MongoKeyValue() {
    clear();
}

MongoKeyValue& MongoKeyValue::operator =(const MongoKeyValue& kv) {
    clear();
    this->ref = kv.ref;
    this->d = kv.d;
    return *this;
}

MongoKeyValue::MongoKeyValue(const std::string &key, const std::string &val) {
    this->d = new Data;
    this->d->type = en_type_str;
    this->d->key = key;
    copy_str(val.c_str(), val.size());
}

MongoKeyValue::MongoKeyValue(const std::string &key, const char *val) {
    this->d = new Data;
    this->d->type = en_type_str;
    this->d->key = key;
    copy_str(val, strlen(val));
}

MongoKeyValue::MongoKeyValue(const std::string &key, int32_t val) {
    this->d = new Data;
    this->d->type = en_type_int32;
    this->d->key = key;
    this->d->meta.i = val;
}

MongoKeyValue::MongoKeyValue(const std::string& key, uint32_t val) {
    this->d = new Data;
    this->d->type = en_type_uint32;
    this->d->key = key;
    this->d->meta.i = (int32_t)val;
}

MongoKeyValue::MongoKeyValue(const std::string& key, int64_t val) {
    this->d = new Data;
    this->d->type = en_type_int64;
    this->d->key = key;
    this->d->meta.i64 = val;
}

MongoKeyValue::MongoKeyValue(const std::string& key, uint64_t val) {
    this->d = new Data;
    this->d->type = en_type_uint64;
    this->d->key = key;
    this->d->meta.i64 = (int64_t)val;
}

MongoKeyValue::MongoKeyValue(const std::string &key, double val) {
    this->d = new Data;
    this->d->type = en_type_double;
    this->d->key = key;
    this->d->meta.f = val;
}

MongoKeyValue::MongoKeyValue(const std::string &key, void *val) {
    // 传进来的val要求是bson_t类型,并且内部会进行自动释放
    this->d = new Data;
    this->d->type = en_type_bson;
    this->d->key = key;
    this->d->meta.bson = val;
}

MongoKeyValue::MongoKeyValue(const std::string& key, bool val) {
    this->d = new Data;
    this->d->type = en_type_bool;
    this->d->key = key;
    this->d->meta.i = val ? 1 : 0;
}

MongoKeyValue::MongoKeyValue(const std::string& key) {
    this->d = new Data;
    this->d->type = en_type_null;
    this->d->key = key;
}

MongoKeyValue::MongoKeyValue(const std::string& key, const char* val, uint32_t size) {
    this->d = new Data;
    this->d->type = en_type_binary;
    this->d->key = key;
    copy_str(val, size);
}

bool MongoKeyValue::is_str() const {
    return (this->d->type == en_type_str);
}

bool MongoKeyValue::is_double() const {
    return (this->d->type == en_type_double);
}

bool MongoKeyValue::is_int() const {
    return (this->d->type == en_type_int32);
}

bool MongoKeyValue::is_uint() const {
    return (this->d->type == en_type_uint32);
}

bool MongoKeyValue::is_bson() const {
    return (this->d->type == en_type_bson);
}

bool MongoKeyValue::is_bool() const {
    return (this->d->type == en_type_bool);
}

bool MongoKeyValue::is_null() const {
    return (this->d->type == en_type_null);
}

bool MongoKeyValue::is_int64() const {
    return (this->d->type == en_type_int64);
}

bool MongoKeyValue::is_uint64() const {
    return (this->d->type == en_type_uint64);
}

bool MongoKeyValue::is_binary() const {
    return (this->d->type == en_type_binary);
}

bool MongoKeyValue::is_array() const {
    return (this->d->type == en_type_array);
}

bool MongoKeyValue::has_cmp() const {
    if (this->d->cmp.empty()) {
        return false;
    }
    return true;
}

void MongoKeyValue::copy_str(const char* val, uint32_t size) {
    this->d->meta.str = (char*)malloc(size + 1);
    memcpy(this->d->meta.str, val, size);
    this->d->meta.str[size] = '\0';
    this->d->str_len = size;
}

void MongoKeyValue::clear() {
    if (this->ref.use_count() == 1) {
        if (this->is_str() || this->is_binary()) {
            free(this->d->meta.str);
        }
        else if (this->is_bson() || this->is_array()) {
            destory_bson((bson_t*)this->d->meta.bson);
        }
        delete this->d;
        this->d = 0;
    }
    this->ref.reset();
}

///////////////////////////////////////////////////////////////////

MongoKeyValueCmp::MongoKeyValueCmp(const std::string &key, int32_t val, const char *cmp) 
    : MongoKeyValue(key, val) {
    if (cmp) {
        this->d->cmp = cmp;
    }
}

MongoKeyValueCmp::MongoKeyValueCmp(const std::string &key, uint32_t val, const char *cmp) 
    : MongoKeyValue(key, val) {
    if (cmp) {
        this->d->cmp = cmp;
    }
}

MongoKeyValueCmp::MongoKeyValueCmp(const std::string &key, int64_t val, const char *cmp) 
    : MongoKeyValue(key, val) {
    if (cmp) {
        this->d->cmp = cmp;
    }
}

MongoKeyValueCmp::MongoKeyValueCmp(const std::string &key, uint64_t val, const char *cmp) 
    : MongoKeyValue(key, val) {
    if (cmp) {
        this->d->cmp = cmp;
    }
}

MongoKeyValueCmp::MongoKeyValueCmp(const std::string &key, double val, const char *cmp) 
    : MongoKeyValue(key, val) {
    if (cmp) {
        this->d->cmp = cmp;
    }
}

}
}