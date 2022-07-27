/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include "serve/serve/base.h"
#include <unordered_map>

namespace http {

bool init(void* event_base, const LinkInfo& link);

void update(uint32_t curTime);

void send(void* request, const char* buf, uint32_t len);

void send(void* request, const std::unordered_map<std::string, std::string>& header, const char* buf, uint32_t len);

const char* decodeUri(const char* s);
}