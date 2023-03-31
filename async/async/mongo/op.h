/*----------------------------------------------------------------
// Copyright 2023
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

namespace async {
namespace mongo {

void onPushRsp(MongoRspDataPtr rsp, void* t);

void opCmd(MongoCorePtr core, MongoReqDataPtr req);

}
}