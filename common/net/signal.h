/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include <functional>
#include <unordered_map>

namespace net {

class Signal {
public:
    struct Signal_Item {
        std::function<void(uint32_t)> f;
        void* evt = 0;
    };

    Signal(void* eventBase);

    virtual ~Signal();

    // 注册处理handler
    bool registe(uint32_t signal, std::function<void(uint32_t)> handler);

private:
    static void onSignal(int signal, short s, void* ctx);

protected:
    void* mEventBase = 0;
    bool mCreatedBase = false;
    std::unordered_map<uint32_t, Signal_Item> mHandlerMap;
};

}