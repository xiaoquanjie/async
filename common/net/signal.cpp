/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_NET

#include "common/net/signal.h"
#include <event.h>
#include <signal.h>

namespace net {

Signal::Signal(void* eventBase) {
    mEventBase = eventBase;
    if (!eventBase) {
        mEventBase = event_base_new();
        mCreatedBase = true;
    }
}

Signal::~Signal() {
    for (auto& item : mHandlerMap) {
        evsignal_del((event*)item.second.evt);
    }
    if (mCreatedBase) {
        event_base_free((event_base*)mEventBase);
    }
}

bool Signal::registe(uint32_t signal, std::function<void(uint32_t)> handler) {
    auto iter = mHandlerMap.find(signal);
    if (iter != mHandlerMap.end()) {
        return false;
    }

    Signal_Item item;
    item.evt = evsignal_new((event_base*)mEventBase, signal, Signal::onSignal, this);
    if (!item.evt) {
        return false;
    }

    auto r = evsignal_add((event*)item.evt, nullptr);
	if (r != 0) {
		return false;
	}

    item.f = handler;
    mHandlerMap[signal] = item;
    return true;
}

void Signal::onSignal(int signal, short /*s*/, void* ctx) {
    Signal* p = reinterpret_cast<Signal*>(ctx);
    auto iter = p->mHandlerMap.find(signal);
    if (iter == p->mHandlerMap.end()) {
        return;
    }

    iter->second.f(signal);
}

}

#endif