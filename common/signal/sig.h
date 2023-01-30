/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include <functional>

namespace sig {

void initSignal(void* eventBase);

bool regSinal(uint32_t signal, std::function<void(uint32_t)> handler);

void regKill(std::function<void(uint32_t)> handler);

void update();

}