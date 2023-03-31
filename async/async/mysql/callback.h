/*----------------------------------------------------------------
// Copyright 2023
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

namespace async {
namespace mysql {

void onPushRsp(MysqlRspDataPtr rsp, void* t);

void onTimeout(MysqlReqDataPtr req, MysqlCorePtr c, void* t);

void onResult(MysqlReqDataPtr req, MysqlCorePtr c, void* res);

}
}