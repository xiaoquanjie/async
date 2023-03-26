/*----------------------------------------------------------------
// Copyright 2023
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_ZOOKEEPER

#include "common/async/zookeeper/data.h"
#include "common/async/zookeeper/callback.h"

namespace async {
namespace zookeeper {

void onPushRsp(ZookRspDataPtr rsp, void* t) {
    ZookThreadData* tData = (ZookThreadData*)t;
    tData->rspLock.lock();
    tData->rspQueue.push_back(rsp);
    tData->rspLock.unlock();
}

void onTimeout(ZookReqDataPtr req, ZookCorePtr c, void* t) {
    if (c) {
        c->task--;
    }
    
    auto rsp = std::make_shared<ZookRspData>();
    rsp->req = req;
    rsp->parser = std::make_shared<ZookParser>(ZOPERATIONTIMEOUT, nullptr, nullptr, 0);
    onPushRsp(rsp, t);
}

void onConnCb(zhandle_t* zh, int type, int state, const char *path, void* v) {
    ZookCore* core = (ZookCore*)v;
    zookLog("id: %s|zh:%p, state: %s, type:%s", core->addr->id.c_str(), zh, state2String(state), type2String(type));

    if (ZOO_CONNECTED_STATE == state) {
        core->conn->connected = true;
        zookLog("id: %s|zh:%p connect ok", core->addr->id.c_str(), zh);
    } else {
        core->conn->connected = false;
        core->conn->auth = false;
        zookLog("id: %s|zh:%p lost connection", core->addr->id.c_str(), zh);
    }
}

void onAuthCb(int rc, const void *data) {
    ZookCore* core = (ZookCore*)data;
    if (!core->conn) {
        zookLog("[error] empty conn!!!!!!!%s", "");
        return;
    }

    if (rc == ZOK) {
        core->conn->auth = 1;
    } else {
        zookLog("[error] add_auth_callback: %s", core->addr->id.c_str());
        core->conn->auth = 0;
    }
}

void onDataCb(int rc, const char *value, int value_len, const struct Stat *stat, const void *data) {
    InvokeData* invoke = (InvokeData*)data;
    invoke->c->task--;
    auto rsp = std::make_shared<ZookRspData>();
    rsp->req = invoke->req;
    delete invoke;

    rsp->parser = std::make_shared<ZookParser>(rc, stat, value, value_len);
    onPushRsp(rsp, rsp->req->tData);
}

void onStringCb(int rc, const char *value, const void *data) {
    InvokeData* invoke = (InvokeData*)data;
    invoke->c->task--;
    auto rsp = std::make_shared<ZookRspData>();
    rsp->req = invoke->req;
    delete invoke;

    // value返回的是节点名
    int len = 0;
    if (rc == ZOK && value) {
        const char* c = value;
        while (*c++ != '\0') len++;
    }
    rsp->parser = std::make_shared<ZookParser>(rc, nullptr, value, len);
    onPushRsp(rsp, rsp->req->tData);
}

void onVoidCb(int rc, const void *data) {
    InvokeData* invoke = (InvokeData*)data;
    invoke->c->task--;
    auto rsp = std::make_shared<ZookRspData>();
    rsp->req = invoke->req;
    delete invoke;

    rsp->parser = std::make_shared<ZookParser>(rc, nullptr, nullptr, 0);
    onPushRsp(rsp, rsp->req->tData);
}

void onStatCb(int rc, const struct Stat *stat, const void *data) {
    InvokeData* invoke = (InvokeData*)data;
    invoke->c->task--;
    auto rsp = std::make_shared<ZookRspData>();
    rsp->req = invoke->req;
    delete invoke;

    rsp->parser = std::make_shared<ZookParser>(rc, stat, nullptr, 0);
    onPushRsp(rsp, rsp->req->tData);
}

void onAclCb(int rc, struct ACL_vector *acl, struct Stat *stat, const void *data) {
    InvokeData* invoke = (InvokeData*)data;
    invoke->c->task--;
    auto rsp = std::make_shared<ZookRspData>();
    rsp->req = invoke->req;
    delete invoke;

    rsp->parser = std::make_shared<ZookParser>(rc, stat, nullptr, 0);
    if (rc == ZOK) {
        rsp->parser->setAcls(acl);
    }

    onPushRsp(rsp, rsp->req->tData);
}

void onStringsCb(int rc, const struct String_vector *strings, const void *data) {
    InvokeData* invoke = (InvokeData*)data;
    invoke->c->task--;
    auto rsp = std::make_shared<ZookRspData>();
    rsp->req = invoke->req;
    delete invoke;

    rsp->parser = std::make_shared<ZookParser>(rc, nullptr, nullptr, 0);
    if (rc == ZOK) {
        rsp->parser->setChilds(strings);
    }

    onPushRsp(rsp, rsp->req->tData);
}

}
}

#endif