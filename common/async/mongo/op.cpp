/*----------------------------------------------------------------
// Copyright 2023
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_MONGO

#include "common/async/mongo/data.h"

namespace async {
namespace mongo {

void onPushRsp(MongoRspDataPtr rsp, void* t) {
    MongoThreadData* tData = (MongoThreadData*)t;
    tData->rspLock.lock();
    tData->rspQueue.push_back(rsp);
    tData->rspLock.unlock();   
}

// insert操作
void onInsertCmd(mongoc_collection_t* c, BaseMongoCmd& cmd, MongoReplyParserPtr parser) {
    if (!mongoc_collection_insert(c,
                                  MONGOC_INSERT_NONE,
                                  (bson_t *)cmd.d->doc_bson_ptr,
                                  nullptr,
                                  (bson_error_t *)parser->error))
    {
        // 失败
        parser->op_result = -1;
    }
}

void onFindCmd(mongoc_collection_t* c, BaseMongoCmd& cmd, MongoReplyParserPtr parser) {
    auto cursor = mongoc_collection_find(c,
                                         MONGOC_QUERY_NONE,
                                         0,
                                         0,
                                         0,
                                         (bson_t *)cmd.d->doc_bson_ptr,
                                         (bson_t *)cmd.d->opt_bson_ptr,
                                         nullptr);
    auto& bson_vec = parser->bsons;
    // mongoc_cursor_next是组塞函数
    const bson_t *doc = 0;
    while (mongoc_cursor_next((mongoc_cursor_t *)cursor, &doc)) {
        // 拷贝一份bson
        bson_vec.push_back(bson_copy(doc));
    }
    if (mongoc_cursor_error((mongoc_cursor_t *)cursor, (bson_error_t *)parser->error)) {
        parser->op_result = -1;
    }
    mongoc_cursor_destroy(cursor);
}

void onFindOptCmd(mongoc_collection_t* c, BaseMongoCmd& cmd, MongoReplyParserPtr parser) {
    auto cursor = mongoc_collection_find_with_opts(c,
                                                   (bson_t *)cmd.d->doc_bson_ptr,
                                                   (bson_t *)cmd.d->opt_bson_ptr,
                                                   nullptr);
    auto& bson_vec = parser->bsons;
    // mongoc_cursor_next是组塞函数
    const bson_t* doc = 0;
    while (mongoc_cursor_next((mongoc_cursor_t*)cursor, &doc)) {
        // 拷贝一份bson
        bson_vec.push_back(bson_copy(doc));
    }
    if (mongoc_cursor_error((mongoc_cursor_t*)cursor, (bson_error_t *)parser->error)) {
        parser->op_result = -1;
    }
    mongoc_cursor_destroy(cursor);
}

void onDeleteOneCmd(mongoc_collection_t* c, BaseMongoCmd& cmd, MongoReplyParserPtr parser) {
    bson_t bson_reply;
    if (!mongoc_collection_delete_one(c,
                                      (bson_t *)cmd.d->doc_bson_ptr,
                                      nullptr,
                                      &bson_reply,
                                      (bson_error_t *)parser->error))
    {
        // 失败
        parser->op_result = -1;
    }
    else {
        auto& bson_vec = parser->bsons;
        bson_vec.push_back(bson_copy(&bson_reply));
    }
}

void onDeleteManyCmd(mongoc_collection_t* c, BaseMongoCmd& cmd, MongoReplyParserPtr parser) {
    bson_t bson_reply;
    if (!mongoc_collection_delete_many(c,
                                       (bson_t *)cmd.d->doc_bson_ptr,
                                       nullptr,
                                       &bson_reply,
                                       (bson_error_t *)parser->error))
    {
        // 失败
        parser->op_result = -1;
    }
    else {
        auto& bson_vec = parser->bsons;
        bson_vec.push_back(bson_copy(&bson_reply));
    }
}

void onUpdateOneCmd(mongoc_collection_t* c, BaseMongoCmd& cmd, MongoReplyParserPtr parser) {
    bson_t bson_reply;
    if (!mongoc_collection_update_one(c,
                                      (bson_t *)cmd.d->doc_bson_ptr,
                                      (bson_t *)cmd.d->update_bson_ptr,
                                      (bson_t *)cmd.d->opt_bson_ptr,
                                      &bson_reply,
                                      (bson_error_t *)parser->error))
    {
        // 失败
        parser->op_result = -1;
    }
    else {
        auto& bson_vec = parser->bsons;
        bson_vec.push_back(bson_copy(&bson_reply));
    }
}

void onUpdateManyCmd(mongoc_collection_t* c, BaseMongoCmd& cmd, MongoReplyParserPtr parser) {
    bson_t bson_reply;
    if (!mongoc_collection_update_many(c,
                                       (bson_t *)cmd.d->doc_bson_ptr,
                                       (bson_t *)cmd.d->update_bson_ptr,
                                       (bson_t *)cmd.d->opt_bson_ptr,
                                       &bson_reply,
                                       (bson_error_t *)parser->error))
    {
        // 失败
        parser->op_result = -1;
    }
    else {
        auto& bson_vec = parser->bsons;
        bson_vec.push_back(bson_copy(&bson_reply));
    }
}

void onCreateIdxCmd(mongoc_collection_t* c, BaseMongoCmd& cmd, MongoReplyParserPtr parser) {
    mongoc_index_opt_t opt;
    mongoc_index_opt_init(&opt);
    opt.unique = cmd.d->unique;
    auto ret = mongoc_collection_create_index(c,
                                              (bson_t *)cmd.d->doc_bson_ptr,
                                              &opt,
                                              (bson_error_t *)parser->error);
    if (!ret) {
        // 失败
        parser->op_result = -1;
    }
}

void onCreateExpireIdx(mongoc_collection_t* c, BaseMongoCmd& cmd, MongoReplyParserPtr parser) {
    mongoc_index_opt_t opt;
    mongoc_index_opt_init(&opt);
    opt.unique = cmd.d->unique;
    opt.expire_after_seconds = cmd.d->ttl;

    auto ret = mongoc_collection_create_index(c,
                                              (bson_t *)cmd.d->doc_bson_ptr,
                                              &opt,
                                              (bson_error_t *)parser->error);
    if (!ret) {
        // 失败
        parser->op_result = -1;
    }
}

void opCmd(MongoCorePtr core, MongoReqDataPtr req) {
    MongoRspDataPtr rsp = std::make_shared<MongoRspData>();
    rsp->req = req;
    rsp->parser = std::make_shared<MongoReplyParser>();
    rsp->parser->op_result = 0;

    do {
        if (!core->conn) {
            bson_set_error((bson_error_t*)rsp->parser->error, 0, 2, "failed to get mongo connection");
            break;
        }

        // 获取集合
        mongoc_collection_t *collection = mongoc_client_get_collection(
            core->conn->mongoc_client,
            req->addr->db.c_str(),
            req->addr->collection.c_str()
        );

        if (!collection) {
            bson_set_error((bson_error_t*)rsp->parser->error, 0, 2, "failed to get mongo collection");
            mongoLog("[error] failed to call mongoc_client_get_collection:%s|%s", req->addr->db.c_str(), req->addr->collection.c_str());
            break;
        }

        if (req->cmd.d->cmd == "insert") {
            onInsertCmd(collection, req->cmd, rsp->parser);
        }
        else if (req->cmd.d->cmd == "find") {
            onFindCmd(collection, req->cmd, rsp->parser);
        }
        else if (req->cmd.d->cmd == "find_opts") {
            onFindOptCmd(collection, req->cmd, rsp->parser);
        }
        else if (req->cmd.d->cmd == "deleteone") {
            onDeleteOneCmd(collection, req->cmd, rsp->parser);
        }
        else if (req->cmd.d->cmd == "deletemany") {
            onDeleteManyCmd(collection, req->cmd, rsp->parser);
        }
        else if (req->cmd.d->cmd == "updateone") {
            onUpdateOneCmd(collection, req->cmd, rsp->parser);
        }
        else if (req->cmd.d->cmd == "updatemany") {
            onUpdateManyCmd(collection, req->cmd, rsp->parser);
        }
        else if (req->cmd.d->cmd == "createidx") {
            onCreateIdxCmd(collection, req->cmd, rsp->parser);
        }
        else if (req->cmd.d->cmd == "createexpireidx") {
            onCreateExpireIdx(collection, req->cmd, rsp->parser);
        }

        mongoc_collection_destroy(collection);
    } while (false);

    core->task--;
    onPushRsp(rsp, req->tData);
}

}
}

#endif