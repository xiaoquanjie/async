/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_MONGO

#include "common/async/mongo/data.h"

namespace async {
namespace mongo {

// io线程的函数
std::function<void(std::function<void()>)> gIoThr = nullptr;
// 最大连接数，连接数越大表现越好
uint32_t gMaxConnection = 100;
// 全局初始化
bool gGlobalIsInit = false;
// 超时时间
uint32_t gTimeout = 5;

///////////////////////////////////////////////////////////////////////

MongoThreadData* runningData() {
    thread_local static MongoThreadData gThrData;
    return &gThrData;
}

bool mongoGlobalInit() {
    if (gGlobalIsInit) {
        return true;
    }

    mongoc_init();
    gGlobalIsInit = true;
    return true;
}

bool mongoGlobalCleanup() {
    mongoc_cleanup();
    return true;
}

///////////////////////////////////////////////////////////////////////

MongoCorePtr threadCreateCore(MongoThreadData* tData, MongoAddrPtr addr) {
    MongoCorePtr core;

    tData->lock.lock();

    auto& pool = tData->corePool[addr->addr];
    if (!pool.valid.empty()) {
        core = pool.valid.front();
        pool.valid.pop_front();
    }

    tData->lock.unlock();

    if (core) {
        return core;
    }

     // 创建新连接
    core = std::make_shared<MongoCore>();
    core->addr = addr;

    // create mongoc uri
    bson_error_t error;
    core->mongoc_uri = mongoc_uri_new_with_error(addr->addr.c_str(), &error);
    if (!core->mongoc_uri) {
        mongoLog("[error] failed to call mongoc_uri_new_with_error:%s", addr->addr.c_str());
        return nullptr;
    }

    core->mongoc_client = mongoc_client_new_from_uri(core->mongoc_uri);
    if (!core->mongoc_client) {
        mongoLog("[error] failed to call mongoc_client_new_from_uri:%s", addr->addr.c_str());
        return nullptr;
    }

    mongoc_client_set_appname(core->mongoc_client, addr->addr.c_str());
    return core;
}

// insert操作
void threadMongoInsert(mongoc_collection_t* mongoc_collection, MongoReqDataPtr req, MongoRspDataPtr rsp) {
    // 判断是否有超时
    if (req->addr->expire_time > 0
        && !req->addr->expire_filed.empty()) {
        bson_append_now_utc((bson_t *)req->cmd.d->doc_bson_ptr,
                            req->addr->expire_filed.c_str(),
                            req->addr->expire_filed.size());
    }

    if (!mongoc_collection_insert(mongoc_collection,
                                  MONGOC_INSERT_NONE,
                                  (bson_t *)req->cmd.d->doc_bson_ptr,
                                  nullptr,
                                  (bson_error_t *)rsp->parser->error))
    {
        // 失败
        rsp->parser->op_result = -1;
    }
}

// find操作
void threadMongoFind(mongoc_collection_t* mongoc_collection, MongoReqDataPtr req, MongoRspDataPtr rsp) {
    auto cursor = mongoc_collection_find(mongoc_collection,
                                         MONGOC_QUERY_NONE,
                                         0,
                                         0,
                                         0,
                                         (bson_t *)req->cmd.d->doc_bson_ptr,
                                         (bson_t *)req->cmd.d->opt_bson_ptr,
                                         nullptr);
    auto& bson_vec = rsp->parser->bsons;
    // mongoc_cursor_next是组塞函数
    const bson_t *doc = 0;
    while (mongoc_cursor_next((mongoc_cursor_t *)cursor, &doc)) {
        // 拷贝一份bson
        bson_vec.push_back(bson_copy(doc));
    }
    if (mongoc_cursor_error((mongoc_cursor_t *)cursor, (bson_error_t *)rsp->parser->error)) {
        rsp->parser->op_result = -1;
    }
    mongoc_cursor_destroy(cursor);
}

// find opt操作
void threadMongoFindOpt(mongoc_collection_t* mongoc_collection, MongoReqDataPtr req, MongoRspDataPtr rsp) {
    auto cursor = mongoc_collection_find_with_opts(mongoc_collection,
                                                   (bson_t *)req->cmd.d->doc_bson_ptr,
                                                   (bson_t *)req->cmd.d->opt_bson_ptr,
                                                   nullptr);
    auto& bson_vec = rsp->parser->bsons;
    // mongoc_cursor_next是组塞函数
    const bson_t* doc = 0;
    while (mongoc_cursor_next((mongoc_cursor_t*)cursor, &doc)) {
        // 拷贝一份bson
        bson_vec.push_back(bson_copy(doc));
    }
    if (mongoc_cursor_error((mongoc_cursor_t*)cursor, (bson_error_t *)rsp->parser->error)) {
            rsp->parser->op_result = -1;
    }
    mongoc_cursor_destroy(cursor);
}

// delete one操作
void threadMongoDeleteOne(mongoc_collection_t* mongoc_collection, MongoReqDataPtr req, MongoRspDataPtr rsp) {
    bson_t bson_reply;
    if (!mongoc_collection_delete_one(mongoc_collection,
                                      (bson_t *)req->cmd.d->doc_bson_ptr,
                                      nullptr,
                                      &bson_reply,
                                      (bson_error_t *)rsp->parser->error))
    {
        // 失败
        rsp->parser->op_result = -1;
    }
    else {
        auto& bson_vec = rsp->parser->bsons;
        bson_vec.push_back(bson_copy(&bson_reply));
    }
}

// delete many操作
void threadMongoDeleteMany(mongoc_collection_t* mongoc_collection, MongoReqDataPtr req, MongoRspDataPtr rsp) {
    bson_t bson_reply;
    if (!mongoc_collection_delete_many(mongoc_collection,
                                       (bson_t *)req->cmd.d->doc_bson_ptr,
                                       nullptr,
                                       &bson_reply,
                                       (bson_error_t *)rsp->parser->error))
    {
        // 失败
        rsp->parser->op_result = -1;
    }
    else {
        auto& bson_vec = rsp->parser->bsons;
        bson_vec.push_back(bson_copy(&bson_reply));
    }
}

// update one
void threadMongoUpdateOne(mongoc_collection_t* mongoc_collection, MongoReqDataPtr req, MongoRspDataPtr rsp) {
    bson_t bson_reply;
    if (!mongoc_collection_update_one(mongoc_collection,
                                      (bson_t *)req->cmd.d->doc_bson_ptr,
                                      (bson_t *)req->cmd.d->update_bson_ptr,
                                      (bson_t *)req->cmd.d->opt_bson_ptr,
                                      &bson_reply,
                                      (bson_error_t *)rsp->parser->error))
    {
        // 失败
        rsp->parser->op_result = -1;
    }
    else {
        auto& bson_vec = rsp->parser->bsons;
        bson_vec.push_back(bson_copy(&bson_reply));
    }
}

// update many
void threadMongoUpdateMany(mongoc_collection_t* mongoc_collection, MongoReqDataPtr req, MongoRspDataPtr rsp) {
    bson_t bson_reply;
    if (!mongoc_collection_update_many(mongoc_collection,
                                       (bson_t *)req->cmd.d->doc_bson_ptr,
                                       (bson_t *)req->cmd.d->update_bson_ptr,
                                       (bson_t *)req->cmd.d->opt_bson_ptr,
                                       &bson_reply,
                                       (bson_error_t *)rsp->parser->error))
    {
        // 失败
        rsp->parser->op_result = -1;
    }
    else {
        auto& bson_vec = rsp->parser->bsons;
        bson_vec.push_back(bson_copy(&bson_reply));
    }
}

// create idx操作
void threadMongoCreateIdx(mongoc_collection_t* mongoc_collection, MongoReqDataPtr req, MongoRspDataPtr rsp) {
    mongoc_index_opt_t opt;
    mongoc_index_opt_init(&opt);
    opt.unique = true;
    auto ret = mongoc_collection_create_index(mongoc_collection,
                                              (bson_t *)req->cmd.d->doc_bson_ptr,
                                              &opt,
                                              (bson_error_t *)rsp->parser->error);
    if (!ret) {
        // 失败
        rsp->parser->op_result = -1;
    }
}

void threadMongoCreateExpireIdx(mongoc_collection_t* mongoc_collection, MongoReqDataPtr req, MongoRspDataPtr rsp) {
    mongoc_index_opt_t opt;
    mongoc_index_opt_init(&opt);
    opt.unique = true;
    // 判断是否有超时
    if (req->addr->expire_time > 0 
        && !req->addr->expire_filed.empty())
    {
        BSON_APPEND_INT32((bson_t *)req->cmd.d->doc_bson_ptr,
                          req->addr->expire_filed.c_str(),
                          1);
        opt.expire_after_seconds = req->addr->expire_time;
    }
    auto ret = mongoc_collection_create_index(mongoc_collection,
                                              (bson_t *)req->cmd.d->doc_bson_ptr,
                                              &opt,
                                              (bson_error_t *)rsp->parser->error);
    if (!ret) {
        // 失败
        rsp->parser->op_result = -1;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void threadProcess(MongoReqDataPtr req, MongoThreadData* tData) {
    MongoRspDataPtr rsp = std::make_shared<MongoRspData>();
    rsp->req = req;
    rsp->parser->op_result = 0;

    MongoCorePtr core;

    do {
        core = threadCreateCore(tData, req->addr);
        if (!core) {
            bson_set_error((bson_error_t*)rsp->parser->error, 0, 2, "failed to get mongo core");
            break;
        }

        mongoc_collection_t *collection = mongoc_client_get_collection(core->mongoc_client,
                                                                       req->addr->db.c_str(),
                                                                       req->addr->collection.c_str());
        if (!collection) {
            bson_set_error((bson_error_t*)rsp->parser->error, 0, 2, "failed to get mongo collection");
            mongoLog("[error] failed to call mongoc_client_get_collection:%s|%s", req->addr->db.c_str(), req->addr->collection.c_str());
            break;
        }

        if (req->cmd.d->cmd == "insert") {
            threadMongoInsert(collection, req, rsp);
        }
        else if (req->cmd.d->cmd == "find") {
            threadMongoFind(collection, req, rsp);
        }
        else if (req->cmd.d->cmd == "find_opts") {
            threadMongoFindOpt(collection, req, rsp);
        }
        else if (req->cmd.d->cmd == "deleteone") {
            threadMongoDeleteOne(collection, req, rsp);
        }
        else if (req->cmd.d->cmd == "deletemany") {
            threadMongoDeleteMany(collection, req, rsp);
        }
        else if (req->cmd.d->cmd == "updateone") {
            threadMongoUpdateOne(collection, req, rsp);
        }
        else if (req->cmd.d->cmd == "updatemany") {
            threadMongoUpdateMany(collection, req, rsp);
        }
        else if (req->cmd.d->cmd == "createidx") {
            threadMongoCreateIdx(collection, req, rsp);
        }
        else if (req->cmd.d->cmd == "createexpireidx") {
            threadMongoCreateExpireIdx(collection, req, rsp);
        }

        mongoc_collection_destroy(collection);
    } while (false);

    tData->lock.lock();

    if (core) {
        // 归还core
        auto& pool = tData->corePool[req->addr->addr];
        pool.valid.push_back(core);
    }

    // 存入队列
    tData->rspQueue.push_back(rsp);
   
    tData->lock.unlock();
}

std::function<void()> localPop(MongoThreadData* tData) {
    if (tData->reqQueue.empty()) {
        assert(false);
        return std::function<void()>();
    }

    auto req = tData->reqQueue.front();
    tData->reqQueue.pop_front();

    auto f = std::bind(threadProcess, req, tData);
    return f;
}

void localRespond(MongoThreadData* tData) {
    if (tData->rspQueue.empty()) {
        return;
    }

    // 交换队列
    std::list<MongoRspDataPtr> rspQueue;
    tData->lock.lock();
    rspQueue.swap(tData->rspQueue);
    tData->lock.unlock();

    for (auto iter = rspQueue.begin(); iter != rspQueue.end(); iter++) {
        tData->rspTaskCnt++;
        auto rsp = *iter;
        rsp->req->cb(rsp->parser);
    }
}

void localProcess(MongoThreadData* tData) {
    if (tData->reqQueue.empty()) {
        return;
    }

    if (gIoThr) {
        gIoThr(localPop(tData));
    } else {
        // 调用线程担任io线程的角色
        localPop(tData)();
    }
}

void localStatistics(MongoThreadData* tData, uint32_t curTime) {
    if (curTime - tData->lastPrintTime < 120) {
        return;
    }

    tData->lastPrintTime = curTime;

    mongoLog("[statistics] cur_task:%d, req_task:%d, rsp_task:%d",
        tData->reqTaskCnt - tData->rspTaskCnt,
        tData->reqTaskCnt,
        tData->rspTaskCnt);

    for (auto& item : tData->corePool) {
        mongoLog("[statistics] id: %s, valid: %d", item.first.c_str(), item.second.valid.size());
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void execute(std::string uri, const BaseMongoCmd& cmd, async_mongo_cb cb) {
    mongoGlobalInit();

    // 构造一个请求
    MongoReqDataPtr req = std::make_shared<MongoReqData>();
    req->cmd = cmd;
    req->addr = std::make_shared<MongoAddr>(uri);
    req->cb = cb;
    time(&req->reqTime);

    MongoThreadData* tData = runningData();
    tData->reqQueue.push_back(req);
    tData->reqTaskCnt++;
}

bool loop(uint32_t curTime) {
    if (!gGlobalIsInit) {
        return false;
    }

    MongoThreadData* tData = runningData();
    localStatistics(tData, curTime);
    localRespond(tData);
    localProcess(tData);

    bool hasTask = tData->reqTaskCnt != tData->rspTaskCnt;
    return hasTask;
}

void setThreadFunc(std::function<void(std::function<void()>)> f) {
    gIoThr = f;
}


} // mongo
} // async

#endif