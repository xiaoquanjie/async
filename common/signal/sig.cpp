/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_NET

#include "common/signal/sig.h"
#include "common/log.h"
#include <unordered_map>
#include <event.h>
#include <signal.h>

namespace sig {

struct SignalItem {
    std::function<void(uint32_t)> f;
    void* evt = 0;
};

struct SignalInfo {
    void* eventBase = 0;
    bool createdBase = false;
    std::unordered_map<uint32_t, SignalItem> handlerMap;

    ~SignalInfo() {
        for (auto& item : handlerMap) {
            evsignal_del((event*)item.second.evt);
        }
        if (createdBase) {
            event_base_free((event_base*)eventBase);
        }

        log("[signal] free");
    }
};

SignalInfo gInfo;

void onSignal(int signal, short s, void* ctx) {
    auto iter = gInfo.handlerMap.find(signal);
    if (iter == gInfo.handlerMap.end()) {
        return;
    }

    iter->second.f(signal);
}

void initSignal(void* eventBase) {
    gInfo.eventBase = eventBase;
    if (!eventBase) {
        gInfo.eventBase = event_base_new();
        gInfo.createdBase = true;
    }
}

bool regSinal(uint32_t signal, std::function<void(uint32_t)> handler) {
    auto iter = gInfo.handlerMap.find(signal);
    if (iter != gInfo.handlerMap.end()) {
        iter->second.f = handler;
        return true;
    }

    SignalItem item;
    item.evt = evsignal_new((event_base*)gInfo.eventBase, signal, onSignal, 0);
    if (!item.evt) {
        return false;
    }

    auto r = evsignal_add((event*)item.evt, nullptr);
	if (r != 0) {
		return false;
	}

    item.f = handler;
    gInfo.handlerMap[signal] = item;
    return true;
}

void regKill(std::function<void(uint32_t)> handler) {
    // ctrl+c
    regSinal(SIGINT, handler);
    // kill -9
    //registe(SIGKILL, handler);
    // kill
    regSinal(SIGTERM, handler);
}

void update() {
    if (gInfo.createdBase) {
        event_base_loop((event_base*)gInfo.eventBase, EVLOOP_NONBLOCK);
    }
}

}

#endif