/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_MONGO

#include "async/async/mongo/data.h"

namespace async {

// 声明
void split(const std::string source, const std::string &separator, std::vector<std::string> &array);

namespace mongo {

MongoAddr::MongoAddr(const std::string& uri) {
    Parse(uri);
}

void MongoAddr::Parse(const std::string &uri) {
    // 解析uri
    std::vector<std::string> values;
    async::split(uri, "|", values);
    if (values.size() == 3) {
        this->host = values[0];
        this->db = values[1];
        this->collection = values[2];
        this->id = this->host + "|" + this->db;
    }
    else {
        mongoLog("[async_mongo] [error] uri error:%s", uri.c_str());
        assert(false);
    }
}

MongoConn::~MongoConn() {
    if (mongoc_uri) {
        mongoc_uri_destroy(mongoc_uri);
    }
    if (mongoc_client) {
        mongoc_client_destroy(mongoc_client);
    }
}

MongoCore::~MongoCore() {
}

CorePoolPtr GlobalData::getPool(const std::string& id) {
    CorePoolPtr pool;
    auto iter = this->corePool.find(id);
    if (iter == this->corePool.end()) {
        pool = std::make_shared<CorePool>();
        this->corePool[id] = pool;
    } else {
        pool = iter->second;
    }
    return pool;
}

}
}
#endif