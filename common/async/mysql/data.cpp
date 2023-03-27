/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_MYSQL

#include "common/async/mysql/data.h"
#include <vector>
#include <assert.h>

namespace async {

// 声明
void split(const std::string source, const std::string &separator, std::vector<std::string> &array);

namespace mysql {

MysqlAddr::MysqlAddr (const std::string& id) {
    Parse(id);
}

void MysqlAddr::Parse(const std::string& id) {
    this->id = id;
    std::vector<std::string> values;
    async::split(id, "|", values);
    if (values.size() == 5) {
        this->host = values[0];
        this->port = atoi(values[1].c_str());
        this->db = values[2];
        this->user = values[3];
        this->pwd = values[4];
    }
    else {
        mysqlLog("[error] uri error:%s", id.c_str());
        assert(false);
    }
}

MysqlConn::~MysqlConn() {
    if (c) {
        mysql_close(c);
    }
}

MysqlCore::~MysqlCore() {
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