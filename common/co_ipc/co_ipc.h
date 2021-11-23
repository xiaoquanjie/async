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

// @io_send_func是真正的发送函数，它的参数：第一个是序列码（作为唤醒协程的凭证）
// @data对方回复的数据
// 返回值是co_bridge::Co_Wait_Return
int send(std::function<void(int64_t)> io_send_func, const char** data, uint32_t& data_len);

// 唤醒
void recv(int64_t sequence_id, const char* data, uint32_t data_len);

};