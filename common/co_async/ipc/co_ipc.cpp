/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#include "common/co_async//ipc/co_ipc.h"
#include "common/co_async/promise.h"
#include <unordered_map>

namespace co_async {
namespace ipc {

uint32_t g_sequence_id = 1;
std::unordered_map<int64_t, co_async::Resolve> g_sequence_map;

std::pair<int, IpcDataPtr> send(std::function<void(uint64_t)> fn, const TimeOut& t) {
    auto res = co_async::promise([fn, t](co_async::Resolve resolve, co_async::Reject) {
        //  生成序列号id
        uint32_t coId = resolve.coId();
        uint32_t id = g_sequence_id++;
        uint64_t seqId = ((uint64_t)id << 32) | coId;

        // 执行真正的发送
        fn(seqId);

        // 添加进序列
        g_sequence_map[seqId] = resolve;

        // 设置一个超时
        co_async::setTimeout([seqId]() {
            // 删除序列信息
            g_sequence_map.erase(seqId);
        }, t() + 1000);

    }, t());

    std::pair<int, IpcDataPtr> ret = std::make_pair(res.first, nullptr);
    if (co_async::checkOk(res)) {
        ret.second = co_async::getOk<IpcData>(res);
    }

    return ret;
}

void recv(uint64_t seqId, const char* data, uint32_t dataLen) {
    auto iter = g_sequence_map.find(seqId);
    if (iter == g_sequence_map.end()) {
        return;
    }

    // 校验信息
    uint32_t coId = seqId & 0xffffffff;
    if (iter->second.coId() != coId) {
        return;
    }

    auto ptr = std::make_shared<IpcData>();
    ptr->data = data;
    ptr->dataLen = dataLen;
    iter->second(ptr);
}


}
}
