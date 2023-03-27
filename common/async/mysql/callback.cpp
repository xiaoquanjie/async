/*----------------------------------------------------------------
// Copyright 2023
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_MYSQL

#include "common/async/mysql/data.h"

namespace async {
namespace mysql {

void onPushRsp(MysqlRspDataPtr rsp, void* t) {
    MysqlThreadData* tData = (MysqlThreadData*)t;
    tData->rspLock.lock();
    tData->rspQueue.push_back(rsp);
    tData->rspLock.unlock();
}

void onTimeout(MysqlReqDataPtr req, MysqlCorePtr c, void* t) {
    if (c) {
        c->task--;
    }

    MysqlRspDataPtr rsp = std::make_shared<MysqlRspData>();
    rsp->req = req;
    rsp->parser = std::make_shared<MysqlReplyParser>(nullptr, 0, true, 0);
    onPushRsp(rsp, req->tData);
}

void onResult(MysqlReqDataPtr req, MysqlCorePtr c, void* res) {
    c->task--;
    int err = mysql_errno(c->conn->c);
    int affect = 0;

    if (err == 0) {
        affect = mysql_affected_rows(c->conn->c); 
    }

    MysqlRspDataPtr rsp = std::make_shared<MysqlRspData>();
    rsp->req = req;
    rsp->parser = std::make_shared<MysqlReplyParser>(res, err, false, affect);
    onPushRsp(rsp, req->tData);
}

}
}

#endif