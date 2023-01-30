/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include "serve/serve/base.h"
#include <functional>

namespace router {

bool init(const LinkInfo& link, const LinkInfo& routes);

bool update(time_t now);

// 添加消息中间件
void use(std::function<bool(uint64_t, uint32_t, const std::string&)> fn);

}