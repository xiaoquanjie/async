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
    // ͨ�����������ң��������������json
    FindMongoCmd(const std::string& query, bool is_idx = true);

    FindMongoCmd(const std::initializer_list<MongoKeyValueCmp> &query_fields,
                 const std::initializer_list<std::string> &include_fields = {},
                 const std::initializer_list<std::string> &except_fields = {});

    // @sort_fiels���������
    // @limit�����Ʒ��ض���������0��Ĭ������
    FindMongoCmd(const std::initializer_list<MongoKeyValueCmp> &query_fields,
                 const std::initializer_list<MongoKeyValue> &sort_fiels,
                 uint32_t limit);
};

/////////////////////////////////////////////////////////////////////////

// ɾ��һ��
struct DeleteOneMongoCmd : public BaseMongoCmd {
    // ͨ�����������ң��������������json
    DeleteOneMongoCmd(const std::string& query, bool is_idx = true);

    // ͨ��field������
    DeleteOneMongoCmd(const std::initializer_list<MongoKeyValueCmp> &option_fields);
};

/////////////////////////////////////////////////////////////////////////

// ɾ������
struct DeleteManyMongoCmd : public BaseMongoCmd {
    // ͨ�����������ң��������������json
    DeleteManyMongoCmd(const std::string& query, bool is_idx = true);

    // ͨ��field������
    DeleteManyMongoCmd(const std::initializer_list<MongoKeyValueCmp> &option_fields);
};

/////////////////////////////////////////////////////////////////////////

// ����һ��
struct UpdateOneMongoCmd : public BaseMongoCmd {
    // @upsert�Ƿ���²���
    UpdateOneMongoCmd(const std::initializer_list<MongoKeyValueCmp> &option_fields,
        const std::initializer_list<MongoKeyValue> &update_fields,
        bool upsert = false);
};

// ���¶���
struct UpdateManyMongoCmd : public BaseMongoCmd {
    // @upsert�Ƿ���²���
    UpdateManyMongoCmd(const std::initializer_list<MongoKeyValueCmp> &option_fields,
        const std::initializer_list<MongoKeyValue> &update_fields,
        bool upsert = false);
};

/////////////////////////////////////////////////////////////////////////

// ��������
struct CreateIndexMongoCmd : public BaseMongoCmd {
    CreateIndexMongoCmd(const std::vector<std::string> &fields);

    CreateIndexMongoCmd(const std::initializer_list<std::string> &fields);
};

// ������ʱ����
struct CreateExpireIndexMongoCmd : public BaseMongoCmd {
    // ����Ҫ��������Ϊ������url�����Դ���
    CreateExpireIndexMongoCmd();
};

}
}


