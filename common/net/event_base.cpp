/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_NET

#include "common/net/event_base.h"
#include <event2/event.h>

namespace net {

void* createEventBase() {
    void* base = event_base_new();
    return base;
}

void freeEventBase(void* base) {
    event_base_free((event_base*)base);
}

}

#endif