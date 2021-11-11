/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include <stdint.h>
#include <functional>

namespace co_async {

typedef std::function<void()> next_cb;
typedef std::function<void(next_cb)> fn_cb;

int parallel(const std::initializer_list<fn_cb>& fns);

int series(const std::initializer_list<fn_cb>& fns);

}

