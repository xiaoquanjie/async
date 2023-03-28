/*----------------------------------------------------------------
// Copyright 2023
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_ZOOKEEPER

#include "common/async/zookeeper/data.h"
#include <cassert>
#include <vector>
#include <string.h>

namespace async {

// 声明
void split(const std::string source, const std::string &separator, std::vector<std::string> &array);

namespace zookeeper {

ZookAddr::ZookAddr(const std::string& id) {
    parse(id);
}

void ZookAddr::parse(const std::string& id) {
    this->id = id;
    std::vector<std::string> values;
    async::split(id, "|", values);

    if (values.size() < 1) {
        zookLog("[error] failed to parse id: %s", id.c_str());
        assert(false);
        return;
    }

    this->host = values[0];
    if (values.size() == 2) {
        this->scheme = values[1];
    }
}

ZookConn::~ZookConn() {
    if (zh) {
        zookeeper_close(zh);
    }
}

ZookCore::~ZookCore() {
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

////////////////////////////////////////////////////////////////////////////////

const char* state2String(int state) {
    if (state == 0) return "CLOSED_STATE";
    if (state == ZOO_CONNECTING_STATE)return "CONNECTING_STATE";
    if (state == ZOO_ASSOCIATING_STATE)return "ASSOCIATING_STATE";
    if (state == ZOO_CONNECTED_STATE)return "CONNECTED_STATE";
    if (state == ZOO_EXPIRED_SESSION_STATE)return "EXPIRED_SESSION_STATE";
    if (state == ZOO_AUTH_FAILED_STATE)return "AUTH_FAILED_STATE";
    return "INVALID_STATE";
}

const char* type2String(int type) {
    if (type == ZOO_CREATED_EVENT)return "CREATED_EVENT";
    if (type == ZOO_DELETED_EVENT)return "DELETED_EVENT";
    if (type == ZOO_CHANGED_EVENT)return "CHANGED_EVENT";
    if (type == ZOO_CHILD_EVENT)return "CHILD_EVENT";
    if (type == ZOO_SESSION_EVENT)return "SESSION_EVENT";
    if (type == ZOO_NOTWATCHING_EVENT)return "NOTWATCHING_EVENT";
    return "UNKNOWN_EVENT_TYPE";
}

const char* lastError() {
    return strerror(errno);
}

bool checkOk(int rc, const char* ctxt) {
    if (rc == ZOK) {
        return true;
    }

    zookLog("[error] failed to call %s: %d", ctxt, rc);
    return false;
}

}
}

#endif