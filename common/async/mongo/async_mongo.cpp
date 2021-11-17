/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_MONGO

#include "common/async/mongo/async_mongo.h"
#include "common/async/mongo/tls.hpp"
#include <queue>
#include <mongoc.h>
#include <list>
#include <mutex>
#include <assert.h>
#include <memory>

namespace async {

// 声明
void split(const std::string source, const std::string &separator, std::vector<std::string> &array);

namespace mongo {

// mongo地址结构
struct mongo_addr {
    // @uri: [mongourl-db-collection]
    mongo_addr(const std::string& uri) {
        Parse(uri);
    }

    mongo_addr() {}

    void Parse(const std::string &uri) {
        // 解析uri
        this->uri = uri;
        std::vector<std::string> values;
        async::split(uri, "|", values);
        if (values.size() == 5) {
            this->addr = values[0];
            this->db = values[1];
            this->collection = values[2];
            this->expire_filed = values[3];
            if (values[4].size()) {
                this->expire_time = std::atoi(values[4].c_str());
            }
        }
        else {
            log("[error] uri error:%s\n", uri.c_str());
            assert(false);
        }
    }

    std::string uri;
    std::string addr;
    std::string db;
    std::string collection;
    std::string expire_filed;
    unsigned int expire_time = 0;
};

// mongo连接核心
struct mongo_core {
    mongo_addr addr;
    mongoc_uri_t* mongoc_uri = 0;
    mongoc_client_t* mongoc_client = 0;

    mongo_core() {}

    ~mongo_core() {
        if (mongoc_uri) {
            mongoc_uri_destroy(mongoc_uri);
        }
        if (mongoc_client) {
            mongoc_client_destroy(mongoc_client);
        }
    }

private:
    // 禁止拷贝
    mongo_core& operator=(const mongo_core&) = delete;
    mongo_core(const mongo_core&) = delete;
};

struct mongo_custom_data {
    mongo_addr addr;
    BaseMongoCmd cmd;
    async_mongo_cb cb;
};

typedef std::shared_ptr<mongo_custom_data> mongo_custom_data_ptr;

struct mongo_respond_data {
    mongo_custom_data_ptr req;
    MongoReplyParserPtr parser;
    mongo_respond_data() {
        parser = std::make_shared<MongoReplyParser>();
    }
};

struct mongo_core_uri_data {
    std::map<std::string, mongo_core*> uri_map;
};

struct mongo_global_data {
    bool init = false;
    std::function<void(std::function<void()>)> thr_func;
    std::queue<mongo_custom_data_ptr> request_queue;
    std::queue<mongo_respond_data> respond_queue;
    std::mutex respond_mutext;
    uint64_t req_task_cnt = 0;
    uint64_t rsp_task_cnt = 0;
};

mongo_global_data g_mongo_global_data;

time_t g_last_statistics_time = 0;

// 日志输出接口
std::function<void(const char*)> g_log_cb = [](const char* data) {
    static std::mutex s_mutex;
    s_mutex.lock();
    printf("[async_mongo] %s\n", data);
    s_mutex.unlock();
};

void log(const char* format, ...) {
    if (!g_log_cb) {
        return;
    }

    char buf[1024] = { 0 };
    va_list ap;
    va_start(ap, format);
    vsprintf(buf, format, ap);
    g_log_cb(buf);
}

///////////////////////////////////////////////////////////////////////

mongo_core* thread_create_core(const mongo_addr& addr) {
    mongo_core_uri_data& uri_data = tlsdata<mongo_core_uri_data, 0>::data();
    auto iter = uri_data.uri_map.find(addr.addr);
    if (iter == uri_data.uri_map.end()) {
        mongo_core* core = new mongo_core;
        core->addr = addr;

        do {
            // create mongoc uri
            bson_error_t error;
            core->mongoc_uri = mongoc_uri_new_with_error(core->addr.addr.c_str(), &error);
            if (!core->mongoc_uri) {
                log("[error] failed to call mongoc_uri_new_with_error:%s", core->addr.addr.c_str());
                break;
            }

            core->mongoc_client = mongoc_client_new_from_uri(core->mongoc_uri);
            if (!core->mongoc_client) {
                log("[error] failed to call mongoc_client_new_from_uri:%s", core->addr.addr.c_str());
                break;
            }

            mongoc_client_set_appname(core->mongoc_client, core->addr.addr.c_str());
            uri_data.uri_map[core->addr.addr] = core;
            return core;
        } while (false);
        
        // release
        delete core;
        return nullptr;
    }

    return iter->second;
}

// insert操作
void thread_mongo_insert_op(mongoc_collection_t* mongoc_collection, mongo_custom_data_ptr req_data, mongo_respond_data* rsp_data) {
    // 判断是否有超时
    if (req_data->addr.expire_time > 0
        && !req_data->addr.expire_filed.empty()) {
        bson_append_now_utc((bson_t *)req_data->cmd.d->doc_bson_ptr,
                            req_data->addr.expire_filed.c_str(),
                            req_data->addr.expire_filed.size());
    }

    if (!mongoc_collection_insert(mongoc_collection,
                                  MONGOC_INSERT_NONE,
                                  (bson_t *)req_data->cmd.d->doc_bson_ptr,
                                  nullptr,
                                  (bson_error_t *)rsp_data->parser->error))
    {
        // 失败
        rsp_data->parser->op_result = -1;
    }
}

// find操作
void thread_mongo_find_op(mongoc_collection_t* mongoc_collection, mongo_custom_data_ptr req_data, mongo_respond_data* rsp_data) {
    auto cursor = mongoc_collection_find(mongoc_collection,
                                         MONGOC_QUERY_NONE,
                                         0,
                                         0,
                                         0,
                                         (bson_t *)req_data->cmd.d->doc_bson_ptr,
                                         (bson_t *)req_data->cmd.d->opt_bson_ptr,
                                         nullptr);
    std::vector<void *> *bson_vec = (std::vector<void *> *)rsp_data->parser->bsons;
    // mongoc_cursor_next是组塞函数
    const bson_t *doc = 0;
    while (mongoc_cursor_next((mongoc_cursor_t *)cursor, &doc)) {
        // 拷贝一份bson
        bson_vec->push_back(bson_copy(doc));
    }
    if (mongoc_cursor_error((mongoc_cursor_t *)cursor, (bson_error_t *)rsp_data->parser->error)) {
        rsp_data->parser->op_result = -1;
    }
    mongoc_cursor_destroy(cursor);
}

// find opt操作
void thread_mongo_find_opts_op(mongoc_collection_t* mongoc_collection, mongo_custom_data_ptr req_data, mongo_respond_data* rsp_data) {
    auto cursor = mongoc_collection_find_with_opts(mongoc_collection,
                                                   (bson_t *)req_data->cmd.d->doc_bson_ptr,
                                                   (bson_t *)req_data->cmd.d->opt_bson_ptr,
                                                   nullptr);
    std::vector<void*>* bson_vec = (std::vector<void*>*)rsp_data->parser->bsons;
    // mongoc_cursor_next是组塞函数
    const bson_t* doc = 0;
    while (mongoc_cursor_next((mongoc_cursor_t*)cursor, &doc)) {
        // 拷贝一份bson
        bson_vec->push_back(bson_copy(doc));
    }
    if (mongoc_cursor_error((mongoc_cursor_t*)cursor, (bson_error_t *)rsp_data->parser->error)) {
            rsp_data->parser->op_result = -1;
    }
    mongoc_cursor_destroy(cursor);
}

// delete one操作
void thread_mongo_deleteone_op(mongoc_collection_t* mongoc_collection, mongo_custom_data_ptr req_data, mongo_respond_data* rsp_data) {
    bson_t bson_reply;
    if (!mongoc_collection_delete_one(mongoc_collection,
                                      (bson_t *)req_data->cmd.d->doc_bson_ptr,
                                      nullptr,
                                      &bson_reply,
                                      (bson_error_t *)rsp_data->parser->error))
    {
        // 失败
        rsp_data->parser->op_result = -1;
    }
    else {
        std::vector<void*>* bson_vec = (std::vector<void*>*)rsp_data->parser->bsons;
        bson_vec->push_back(bson_copy(&bson_reply));
    }
}

// delete many操作
void thread_mongo_deletemany_op(mongoc_collection_t* mongoc_collection, mongo_custom_data_ptr req_data, mongo_respond_data* rsp_data) {
    bson_t bson_reply;
    if (!mongoc_collection_delete_many(mongoc_collection,
                                       (bson_t *)req_data->cmd.d->doc_bson_ptr,
                                       nullptr,
                                       &bson_reply,
                                       (bson_error_t *)rsp_data->parser->error))
    {
        // 失败
        rsp_data->parser->op_result = -1;
    }
    else {
        std::vector<void*>* bson_vec = (std::vector<void*>*)rsp_data->parser->bsons;
        bson_vec->push_back(bson_copy(&bson_reply));
    }
}

// update one
void thread_mongo_updateone_op(mongoc_collection_t* mongoc_collection, mongo_custom_data_ptr req_data, mongo_respond_data* rsp_data) {
    bson_t bson_reply;
    if (!mongoc_collection_update_one(mongoc_collection,
                                      (bson_t *)req_data->cmd.d->doc_bson_ptr,
                                      (bson_t *)req_data->cmd.d->update_bson_ptr,
                                      (bson_t *)req_data->cmd.d->opt_bson_ptr,
                                      &bson_reply,
                                      (bson_error_t *)rsp_data->parser->error))
    {
        // 失败
        rsp_data->parser->op_result = -1;
    }
    else {
        std::vector<void*>* bson_vec = (std::vector<void*>*)rsp_data->parser->bsons;
        bson_vec->push_back(bson_copy(&bson_reply));
    }
}

// update many
void thread_mongo_updatemany_op(mongoc_collection_t* mongoc_collection, mongo_custom_data_ptr req_data, mongo_respond_data* rsp_data) {
    bson_t bson_reply;
    if (!mongoc_collection_update_many(mongoc_collection,
                                       (bson_t *)req_data->cmd.d->doc_bson_ptr,
                                       (bson_t *)req_data->cmd.d->update_bson_ptr,
                                       (bson_t *)req_data->cmd.d->opt_bson_ptr,
                                       &bson_reply,
                                       (bson_error_t *)rsp_data->parser->error))
    {
        // 失败
        rsp_data->parser->op_result = -1;
    }
    else {
        std::vector<void*>* bson_vec = (std::vector<void*>*)rsp_data->parser->bsons;
        bson_vec->push_back(bson_copy(&bson_reply));
    }
}

// create idx操作
void thread_mongo_createidx_op(mongoc_collection_t* mongoc_collection, mongo_custom_data_ptr req_data, mongo_respond_data* rsp_data) {
    mongoc_index_opt_t opt;
    mongoc_index_opt_init(&opt);
    opt.unique = true;
    auto ret = mongoc_collection_create_index(mongoc_collection,
                                              (bson_t *)req_data->cmd.d->doc_bson_ptr,
                                              &opt,
                                              (bson_error_t *)rsp_data->parser->error);
    if (!ret)
    {
        // 失败
        rsp_data->parser->op_result = -1;
    }
}

void thread_mongo_createexpireidx_op(mongoc_collection_t* mongoc_collection, mongo_custom_data_ptr req_data, mongo_respond_data* rsp_data) {
    mongoc_index_opt_t opt;
    mongoc_index_opt_init(&opt);
    opt.unique = true;
    // 判断是否有超时
    if (req_data->addr.expire_time > 0 
        && !req_data->addr.expire_filed.empty())
    {
        BSON_APPEND_INT32((bson_t *)req_data->cmd.d->doc_bson_ptr,
                          req_data->addr.expire_filed.c_str(),
                          1);
        opt.expire_after_seconds = req_data->addr.expire_time;
    }
    auto ret = mongoc_collection_create_index(mongoc_collection,
                                              (bson_t *)req_data->cmd.d->doc_bson_ptr,
                                              &opt,
                                              (bson_error_t *)rsp_data->parser->error);
    if (!ret)
    {
        // 失败
        rsp_data->parser->op_result = -1;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void thread_mongo_process(mongo_custom_data_ptr req_data, mongo_global_data* global_data) {
    mongo_respond_data rsp_data;
    rsp_data.req = req_data;
    rsp_data.parser->op_result = 0;

    do {
        auto core = thread_create_core(req_data->addr);
        if (!core) {
            bson_set_error((bson_error_t*)rsp_data.parser->error, 0, 2, "failed to get mongo core");
            break;
        }

        mongoc_collection_t *collection = mongoc_client_get_collection(core->mongoc_client,
                                                                       req_data->addr.db.c_str(),
                                                                       req_data->addr.collection.c_str());
        if (!collection) {
            bson_set_error((bson_error_t*)rsp_data.parser->error, 0, 2, "failed to get mongo collection");
            log("[error] failed to call mongoc_client_get_collection:%s|%s\n",
                                   req_data->addr.db.c_str(),
                                   req_data->addr.collection.c_str());
            break;
        }

        if (req_data->cmd.d->cmd == "insert") {
            thread_mongo_insert_op(collection, req_data, &rsp_data);
        }
        else if (req_data->cmd.d->cmd == "find") {
            thread_mongo_find_op(collection, req_data, &rsp_data);
        }
        else if (req_data->cmd.d->cmd == "find_opts") {
            thread_mongo_find_opts_op(collection, req_data, &rsp_data);
        }
        else if (req_data->cmd.d->cmd == "deleteone") {
            thread_mongo_deleteone_op(collection, req_data, &rsp_data);
        }
        else if (req_data->cmd.d->cmd == "deletemany") {
            thread_mongo_deletemany_op(collection, req_data, &rsp_data);
        }
        else if (req_data->cmd.d->cmd == "updateone") {
            thread_mongo_updateone_op(collection, req_data, &rsp_data);
        }
        else if (req_data->cmd.d->cmd == "updatemany") {
            thread_mongo_updatemany_op(collection, req_data, &rsp_data);
        }
        else if (req_data->cmd.d->cmd == "createidx") {
            thread_mongo_createidx_op(collection, req_data, &rsp_data);
        }
        else if (req_data->cmd.d->cmd == "createexpireidx") {
            thread_mongo_createexpireidx_op(collection, req_data, &rsp_data);
        }

        mongoc_collection_destroy(collection);
    } while (false);

    // 存入队列
    global_data->respond_mutext.lock();
    global_data->respond_queue.push(rsp_data);
    global_data->respond_mutext.unlock();
}

inline std::function<void()> get_thread_func() {
    // 取一条
    assert(g_mongo_global_data.request_queue.size() > 0);
    mongo_custom_data_ptr req_data = g_mongo_global_data.request_queue.front();
    g_mongo_global_data.request_queue.pop();

    auto f = std::bind(thread_mongo_process,
                       req_data,
                       &g_mongo_global_data);
    return f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool local_mongo_init() {
    if (g_mongo_global_data.init) {
        return true;
    }

    mongoc_init();
    g_mongo_global_data.init = true;
    return true;
}

bool local_mongo_cleanup() {
    mongoc_cleanup();
    return true;
}
 
void execute(std::string uri, const BaseMongoCmd& cmd, async_mongo_cb cb) {
    local_mongo_init();
    // 构造一个请求
    mongo_custom_data_ptr req = std::make_shared<mongo_custom_data>();
    req->cmd = cmd;
    req->addr.Parse(uri);
    req->cb = cb;
    g_mongo_global_data.request_queue.push(req);
    g_mongo_global_data.req_task_cnt++;
}

void statistics() {
    if (!g_log_cb) {
        return;
    }

    time_t now = 0;
    time(&now);
    if (now - g_last_statistics_time <= 120) {
        return;
    }

    // 没有输出连接池大小
    g_last_statistics_time = now;
    log("[statistics] cur_task:%d, req_task:%d, rsp_task:%d",
        (g_mongo_global_data.req_task_cnt - g_mongo_global_data.rsp_task_cnt),
        g_mongo_global_data.req_task_cnt,
        g_mongo_global_data.rsp_task_cnt);
}

void local_process_respond() {
    if (g_mongo_global_data.respond_queue.empty()) {
        return;
    }

    // 交换队列
    g_mongo_global_data.respond_mutext.lock();
    std::queue<mongo_respond_data> respond_queue;
    g_mongo_global_data.respond_queue.swap(respond_queue);
    g_mongo_global_data.respond_mutext.unlock();

    while (!respond_queue.empty()) {
        g_mongo_global_data.rsp_task_cnt++;
        mongo_respond_data rsp = std::move(respond_queue.front());
        respond_queue.pop();
        rsp.req->cb(rsp.parser);
    }
}

void local_process_task() {
    if (!g_mongo_global_data.request_queue.empty()) {
        if (g_mongo_global_data.thr_func) {
            g_mongo_global_data.thr_func(get_thread_func());
        }
        else {
            get_thread_func()();
        }
    }
}

bool loop() {
    if (!g_mongo_global_data.init) {
        return false;
    }

    local_process_task();
    local_process_respond();
    statistics();

    // 是否有任务
    bool has_task = false;
    if (g_mongo_global_data.req_task_cnt != g_mongo_global_data.rsp_task_cnt) {
        has_task = true;
    }
    return has_task;
}

void setThreadFunc(std::function<void(std::function<void()>)> f) {
    g_mongo_global_data.thr_func = f;
}

} // mongo
} // async

#endif