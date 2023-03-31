/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace async {

namespace mongo {

struct MongoKeyValue;
struct MongoKeyValueCmp;

void destory_bson(void* p);

void* new_from_json(const std::string& json);

void *new_from_query(const std::string &query, bool is_idx);

void bson_append_idx(void *b, const std::string &idx);

void bson_append(void *b, const char *field, const std::string& val);

void bson_append(void *b, const char *field, const char *val);

void bson_append(void *b, const char *field, int32_t val);

void bson_append(void *b, const char *field, uint32_t val);

void bson_append(void *b, const char *field, int64_t val);

void bson_append(void *b, const char *field, uint64_t val);

void bson_append(void *b, const char *field, double val);

// val表示的是bson数据
void bson_append(void *b, const char *field, void *val);

void bson_append(void *b, const char *field, bool val);

// 二进制
void bson_append(void *b, const char *field, const char *val, size_t size);

// null
void bson_append(void *b, const char *field);

void bson_append(void *b, const char *cmp, const MongoKeyValue &field);

void bson_append(void *b, const char *field, const std::initializer_list<MongoKeyValue> &fields);

void bson_append(void *b, const std::initializer_list<MongoKeyValue> &fields);

void bson_append(void *b, const std::initializer_list<MongoKeyValueCmp> &fields);

void bson_append(void *b, const std::initializer_list<std::string> &fields, bool include);

void bson_array_append(void *b, const char *field, void* val);
//////////////////////////////////////////////////////////////////////////////////////////////

// 键值对
struct MongoKeyValue {
    // 数据类型
    enum {
        en_type_str = 0,
        en_type_double = 1,
        en_type_int32 = 2,
        en_type_bson = 3,
        en_type_bool = 4,
        en_type_null = 5,
        en_type_uint32 = 6,
        en_type_int64 = 7,
        en_type_uint64 = 8,
        en_type_binary = 9,
        en_type_array = 10,
    };

    struct Data {
        int type;
        std::string key;
        std::string cmp;
        union {
            int32_t i;
            int64_t i64;
		    double f;
            void* bson;  
		    char* str;
        } meta;
        uint32_t str_len = 0;

        Data();
        ~Data();
    };

    std::shared_ptr<Data> d;
    
    ~MongoKeyValue();

    // 字符串
    MongoKeyValue(const std::string& key, const std::string& val);

    // 字符串
    MongoKeyValue(const std::string& key, const char* val);

    MongoKeyValue(const std::string& key, int32_t val);

    MongoKeyValue(const std::string& key, uint32_t val);

    MongoKeyValue(const std::string& key, int64_t val);

    MongoKeyValue(const std::string& key, uint64_t val);

    MongoKeyValue(const std::string& key, double val);

    MongoKeyValue(const std::string& key, void* val);

    MongoKeyValue(const std::string& key, bool val);

    MongoKeyValue(const std::string& key);

    // 二进制
    MongoKeyValue(const std::string& key, const char* val, uint32_t size);

    // 数组
    template<typename T>
    MongoKeyValue(const std::string& key, const std::initializer_list<T>& array) {
        std::vector<T> vec;
        for (auto& item : array) {
            vec.push_back(item);
        }

        void* b = new_from_json("");
        for (size_t idx = 0; idx < vec.size(); ++idx) {
            bson_append(b, std::to_string(idx).c_str(), vec[idx]);
        }
        
        this->d = std::make_shared<Data>();
        this->d->type = en_type_array;
        this->d->key = key;
        this->d->meta.bson = b;
    }

    template<typename T>
    MongoKeyValue(const std::string& key, const std::vector<T>& array) {
        void* b = new_from_json("");
        for (size_t idx = 0; idx < array.size(); ++idx) {
            bson_append(b, std::to_string(idx).c_str(), array[idx]);
        }
        
        this->d = std::make_shared<Data>();
        this->d->type = en_type_array;
        this->d->key = key;
        this->d->meta.bson = b;
    }

    bool is_str() const;

    bool is_double() const;

    bool is_int() const;

    bool is_uint() const;

    bool is_bson() const;

    bool is_bool() const;

    bool is_null() const;

    bool is_int64() const;

    bool is_uint64() const;

    bool is_binary() const;

    bool is_array() const;

    bool has_cmp() const;

protected:
    // 禁止默认构造函数
    MongoKeyValue() = delete;

    void copy_str(const char* val, uint32_t size);
};

//////////////////////////////////////////////////////////////////

// 带比较条件的键值对
struct MongoKeyValueCmp : public MongoKeyValue {
    MongoKeyValueCmp(const std::string& key, const std::string& val) 
        : MongoKeyValue(key, val) {}

    MongoKeyValueCmp(const std::string& key, const char* val) 
        : MongoKeyValue(key, val) {}

    MongoKeyValueCmp(const std::string& key, void* val)
        : MongoKeyValue(key, val) {}

    MongoKeyValueCmp(const std::string& key, bool val)
        : MongoKeyValue(key, val) {}

    // @cmp为空时，表示比较条件是相等
    MongoKeyValueCmp(const std::string& key, int32_t val, const char* cmp = nullptr);

    MongoKeyValueCmp(const std::string& key, uint32_t val, const char* cmp = nullptr);

    MongoKeyValueCmp(const std::string& key, int64_t val, const char* cmp = nullptr);

    MongoKeyValueCmp(const std::string& key, uint64_t val, const char* cmp = nullptr);

    MongoKeyValueCmp(const std::string& key, double val, const char* cmp = nullptr);

    template<typename T>
    MongoKeyValueCmp(const std::string& key, const std::vector<T>& array, const char* cmp = nullptr) 
       : MongoKeyValue(key, array) {
        if (cmp) {
            this->d->cmp = cmp;
        }
    }

    template<typename T>
    MongoKeyValueCmp(const std::string& key, const std::initializer_list<T>& array, const char* cmp = nullptr) 
       : MongoKeyValue(key, array) {
        if (cmp) {
            this->d->cmp = cmp;
        }
    }
};

} // mongo
} // async