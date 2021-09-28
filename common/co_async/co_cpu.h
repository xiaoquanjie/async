/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include "common/async/cpu/async_cpu.h"
#include "common/co_async/co_io.h"

namespace co_async {
namespace cpu {

int get_wait_time();

void set_wait_time(int wait_time);

int execute(async::cpu::async_cpu_op op, void* user_data, async::cpu::async_cpu_cb cb);

}
}