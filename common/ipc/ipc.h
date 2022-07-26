/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include "common/ipc/zero_mq_unit.h"
#include "common/ipc/zero_mq_handler.h"
#include <functional>

namespace ipc {

void setLogFunc(std::function<void(const char*)> cb);

void log(const char* format, ...);

}