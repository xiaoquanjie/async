/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include "common/async/cpu/async_cpu.h"

namespace co_async {
namespace cpu {

std::pair<int, int64_t> execute(async::cpu::async_cpu_op op, void* userData, int timeOut = 30 * 1000);

}
}