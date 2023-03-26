/*----------------------------------------------------------------
// Copyright 2023
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

namespace async {
namespace curl {

void onPushRsp(CurlRspDataPtr rsp, void* t);

size_t onWriteCb(void *buffer, size_t size, size_t count, void* param);

}
}