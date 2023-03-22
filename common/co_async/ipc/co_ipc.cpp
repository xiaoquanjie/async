/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#include "common/co_async//ipc/co_ipc.h"
#include "common/co_async/promise.h"
#include <unordered_map>

namespace co_async {

std::shared_ptr<UniqueInfo> getByUniqueId(uint64_t id);

namespace ipc {

std::pair<int, IpcDataPtr> send(std::function<void(uint64_t)> fn, const TimeOut& t) {
    auto res = co_async::promise([fn, t](co_async::Resolve resolve, co_async::Reject) {
        //  取出序列号id
        uint64_t seqId = resolve.result->uniqueId;

        // 执行真正的发送
        fn(seqId);

    }, t());

    std::pair<int, IpcDataPtr> ret = std::make_pair(res.first, nullptr);
    if (co_async::checkOk(res)) {
        ret.second = co_async::getOk<IpcData>(res);
    }

    return ret;
}

void recv(uint64_t seqId, const char* data, uint32_t dataLen) {
    auto info = getByUniqueId(seqId);
    if (info == nullptr) {
        return;
    }
    
    // 校验信息
    uint32_t coId = seqId & 0xffffffff;
    if (info->resolve.coId() != coId) {
        return;
    }

    auto ptr = std::make_shared<IpcData>();
    ptr->data = data;
    ptr->dataLen = dataLen;

    // 唤醒
    info->resolve(ptr);
}


}
}
