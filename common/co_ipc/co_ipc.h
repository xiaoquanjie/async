/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include <functional>

namespace co_ipc {

int getWaitTime();

void setWaitTime(int wait_time);

// @io_send_func的参数：第一个是序列码（作为唤醒协程的凭证
// 返回值是co_bridge::Co_Wait_Return
int send(std::function<void(int64_t)> io_send_func);

// 唤醒
void resume(int64_t sequence_id);

};