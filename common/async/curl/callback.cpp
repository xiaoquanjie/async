/*----------------------------------------------------------------
// Copyright 2023
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_CURL

#include "common/async/curl/data.h"

namespace async {
namespace curl {

void onPushRsp(CurlRspDataPtr rsp, void* t) {
    CurlThreadData* tData = (CurlThreadData*)t;
    tData->rspLock.lock();
    tData->rspQueue.push_back(rsp);
    tData->rspLock.unlock();
}

size_t onWriteCb(void *buffer, size_t size, size_t count, void* param) {
    std::string* s = static_cast<std::string*>(param);
    if (nullptr == s) {
        return 0;
    }

    s->append(reinterpret_cast<char*>(buffer), size * count);
    return size * count;
}

}
}

#endif