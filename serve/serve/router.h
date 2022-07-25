/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include "serve/serve/base.h"

namespace router {

bool init(const World& self, const LinkInfo& link, const std::vector<World>& routes);

}