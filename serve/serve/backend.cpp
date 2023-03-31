/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_IPC

#include "serve/serve/backend.h"
#include "async/ipc/ipc.h"
#include "async/transaction/transaction_mgr.h"
#include <assert.h>

namespace backend {

std::function<bool(uint64_t, uint32_t, const std::string&)> gMiddle = nullptr;

struct ZqDealer : public ipc::ZeromqDealerHandler {
    void onData(uint64_t uniqueId, uint32_t identify, const std::string& data) override {
        if (!(gMiddle && gMiddle(uniqueId, identify, data))) {
            BackendMsg message;
            message.decode(data);

            if (message.header.rspSeqId == 0) {
#ifdef USE_TRANS
                trans_mgr::handle(message.header.cmd, (char*)&message, 0, (void*)0);
#endif
            }
            else {
                co_async::ipc::recv(message.header.rspSeqId, (char*)&message, 0);
            }
        }
    }
};

ZqDealer gDealer;
LinkInfo gLinkInfo;
World gSelf;

bool init(const World& self, const LinkInfo& link) {
    gLinkInfo = link;
    gSelf = self;

    for (auto& item : gLinkInfo.itemVec) {
        uint64_t id = gDealer.connect(gSelf.identify(), item.addr());
        if (id == 0) {
            return false;
        }
        item.uniqueId = id;
    }
    return true;
}

bool update(time_t now) {
    return gDealer.update(now);
}

void use(std::function<bool(uint64_t, uint32_t, const std::string&)> fn) {
    gMiddle = fn;
}

uint32_t selfWorld() {
    return gSelf.world;
}

uint32_t selfWorldId() {
    return gSelf.identify();
}

void send(uint64_t uniqueId, const std::string& data) {
    gDealer.sendData(uniqueId, data);
}

void send(BackendMsg& frame) {
    // 挑选出一个路由
    if (gLinkInfo.itemVec.empty()) {
        assert(false);
        return;
    }

    std::string output;
    frame.header.cmdLength = frame.data.size();
    frame.encode(output);

    size_t idx = frame.header.targetId % gLinkInfo.itemVec.size();
    uint64_t uniqueId = gLinkInfo.itemVec[idx].uniqueId;

    send(uniqueId, output);
}

}

#endif