/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include <functional>
#include "async/co_async/comm.hpp"

namespace co_async {
namespace ipc {

struct IpcData {
    const char* data = 0;
    uint32_t dataLen = 0;
};

typedef std::shared_ptr<IpcData> IpcDataPtr;

// fn是真正的发送函数，它的参数：第一个是序列码（作为唤醒协程的凭证）
std::pair<int, IpcDataPtr> send(std::function<void(uint64_t)> fn, const TimeOut& t = TimeOut(5 * 1000));

// 唤醒
void recv(uint64_t seqId, const char* data, uint32_t dataLen);

}
}