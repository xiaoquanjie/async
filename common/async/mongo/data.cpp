/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_MONGO

#include "common/async/mongo/data.h"

namespace async {
namespace mongo {

MongoAddr::MongoAddr(const std::string& uri) {
    Parse(uri);
}

void MongoAddr::Parse(const std::string &uri) {
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
        mongoLog("[async_mongo] [error] uri error:%s", uri.c_str());
        assert(false);
    }
}

MongoCore::~MongoCore() {
    if (mongoc_uri) {
        mongoc_uri_destroy(mongoc_uri);
    }
    if (mongoc_client) {
        mongoc_client_destroy(mongoc_client);
    }
}

MongoRspData::MongoRspData() {
    parser = std::make_shared<MongoReplyParser>();
}


}
}
#endif