/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include "common/async/mongo/mongo_struct.h"

namespace async {

namespace mongo {

struct BaseMongoCmd {
    struct Data {
        std::string cmd;
        void* doc_bson_ptr = 0;
        void* update_bson_ptr = 0;
        void* opt_bson_ptr = 0;

        ~Data();
    };

    BaseMongoCmd();

    ~BaseMongoCmd();

    bool IsDeleteCmd() const;

    bool IsUpdateCmd() const;

    bool IsInsertCmd() const;

    std::string DebugString() const;

    std::shared_ptr<Data> d;
};

/////////////////////////////////////////////////////////////////////////

struct InsertMongoCmd : public BaseMongoCmd {
    InsertMongoCmd(const std::string& json);

    InsertMongoCmd(const std::initializer_list<MongoKeyValue> &fields);
};

/////////////////////////////////////////////////////////////////////////

struct FindMongoCmd : public BaseMongoCmd {
    // 通过索引来查找，如果不索引就是json
    FindMongoCmd(const std::string& query, bool is_idx = true);

    FindMongoCmd(const std::initializer_list<MongoKeyValueCmp> &query_fields,
                 const std::initializer_list<std::string> &include_fields = {},
                 const std::initializer_list<std::string> &except_fields = {});

    // @sort_fiels是排序参数
    // @limit是限制返回多少条，填0是默认所有
    FindMongoCmd(const std::initializer_list<MongoKeyValueCmp> &query_fields,
                 const std::initializer_list<MongoKeyValue> &sort_fiels,
                 uint32_t limit);
};

/////////////////////////////////////////////////////////////////////////

// 删除一条
struct DeleteOneMongoCmd : public BaseMongoCmd {
    // 通过索引来查找，如果不索引就是json
    DeleteOneMongoCmd(const std::string& query, bool is_idx = true);

    // 通过field来查找
    DeleteOneMongoCmd(const std::initializer_list<MongoKeyValueCmp> &option_fields);
};

/////////////////////////////////////////////////////////////////////////

// 删除多条
struct DeleteManyMongoCmd : public BaseMongoCmd {
    // 通过索引来查找，如果不索引就是json
    DeleteManyMongoCmd(const std::string& query, bool is_idx = true);

    // 通过field来查找
    DeleteManyMongoCmd(const std::initializer_list<MongoKeyValueCmp> &option_fields);
};

/////////////////////////////////////////////////////////////////////////

// 更新一条
struct UpdateOneMongoCmd : public BaseMongoCmd {
    // @upsert是否更新插入
    UpdateOneMongoCmd(const std::initializer_list<MongoKeyValueCmp> &option_fields,
        const std::initializer_list<MongoKeyValue> &update_fields,
        bool upsert = false);
};

// 更新多条
struct UpdateManyMongoCmd : public BaseMongoCmd {
    // @upsert是否更新插入
    UpdateManyMongoCmd(const std::initializer_list<MongoKeyValueCmp> &option_fields,
        const std::initializer_list<MongoKeyValue> &update_fields,
        bool upsert = false);
};

/////////////////////////////////////////////////////////////////////////

// 创建索引
struct CreateIndexMongoCmd : public BaseMongoCmd {
    CreateIndexMongoCmd(const std::vector<std::string> &fields);

    CreateIndexMongoCmd(const std::initializer_list<std::string> &fields);
};

// 创建超时索引
struct CreateExpireIndexMongoCmd : public BaseMongoCmd {
    // 不需要参数，因为参数都url中已自带了
    CreateExpireIndexMongoCmd();
};

}
}


