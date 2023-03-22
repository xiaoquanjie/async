/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include "common/async/cpu/async_cpu.h"
#include "common/co_async/comm.hpp"

namespace co_async {
namespace cpu {

std::pair<int, int64_t> execute(async::cpu::async_cpu_op op, void* userData, const TimeOut& t = TimeOut());

}
}