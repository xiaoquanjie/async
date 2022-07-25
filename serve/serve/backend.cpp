/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_IPC

#include "serve/serve/backend.h"
#include "common/ipc/zero_mq_handler.h"
#include "common/transaction/transaction_mgr.h"
#include "common/co_async/ipc/co_ipc.h"
#include <google/protobuf/message.h>

namespace backend {

struct ZqDealer : public ZeromqDealerHandler {
    void onData(uint64_t uniqueId, uint32_t identify, const std::string& data) override {
        BackendMsg message;
        message.decode(data);

        message.header.localFd = uniqueId;
        message.header.remoteFd = 0;

        if (message.header.rspSeqId == 0) {
            trans_mgr::handle(message.header.cmd, (char*)&message, 0, (void*)0);
        }
        else {
            co_async::ipc::recv(message.header.rspSeqId, (char*)&message, 0);
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

void send(BackendMsg& frame) {
    // 挑选出一个路由
    if (gLinkInfo.itemVec.empty()) {
        assert(false);
        return;
    }

    std::string output;
    frame.encode(output);

    size_t idx = frame.header.targetId % gLinkInfo.itemVec.size();
    uint64_t uniqueId = gLinkInfo.itemVec[idx].uniqueId;

    gDealer.sendData(uniqueId, output);
}

void notifyMsg(World w, uint64_t targetId, const google::protobuf::Message& msg, uint32_t cmdId) {
    uint32_t dstWorldId = w.identify();
    notifyBack(dstWorldId, targetId, msg, cmdId, 0, 0);
}

void notifyMsg(uint32_t type, uint64_t targetId, const google::protobuf::Message& msg, uint32_t cmdId) {
    World w;
    w.world = gSelf.world; // 默认是同一个世界的
    w.type = type;
    w.id = 0;
    notifyMsg(w, targetId, msg, cmdId);
}

void notifyBack(uint32_t worldId, uint64_t targetId, const google::protobuf::Message& msg, uint32_t cmdId, uint64_t rspSeqId, uint32_t result, uint32_t broadcast) {
    BackendMsg frame;
    frame.header.srcWorldId = gSelf.identify();
    frame.header.dstWorldId = worldId;
    frame.header.targetId = targetId;
    frame.header.cmd = cmdId;
    frame.header.rspSeqId = rspSeqId;
    frame.header.result = result;
    frame.header.broadcast = broadcast;

    // 序列化msg
    msg.SerializeToString(&frame.data);
    frame.header.cmdLength = frame.data.size();

    send(frame);
}

void broadcast(uint32_t type, uint64_t targetId, const google::protobuf::Message& msg, uint32_t cmdId) {
    World w;
    w.world = 0;
    w.type = type;
    w.id = 0;
    notifyBack(w.identify(), targetId, msg, cmdId, 0, 0, 1);
}

std::pair<int, uint32_t> sendMsg(World w, uint64_t targetId, const google::protobuf::Message& reqMsg, uint32_t reqCmdId, google::protobuf::Message& rspMsg, int timeOut) {
    // 挑选出一个路由
    if (gLinkInfo.itemVec.empty()) {
        assert(false);
        return std::make_pair(co_async::E_ERROR, 0);;
    }

    size_t idx = targetId % gLinkInfo.itemVec.size();
    uint64_t uniqueId = gLinkInfo.itemVec[idx].uniqueId;

    BackendMsg frame;
    frame.header.srcWorldId = gSelf.identify();
    frame.header.dstWorldId = w.identify();
    frame.header.targetId = targetId;
    frame.header.cmd = reqCmdId;
    
    // 序列化msg
    reqMsg.SerializeToString(&frame.data);
    frame.header.cmdLength = frame.data.size();

    auto res = co_async::ipc::send([&frame, uniqueId](uint64_t seqId) {
        frame.header.reqSeqId = seqId;
        std::string output;
        frame.encode(output);
        
        gDealer.sendData(uniqueId, output);
    }, co_async::TimeOut(timeOut));

    std::pair<int, uint32_t> ret = std::make_pair(res.first, 0);

    if (res.first == co_async::E_OK) {
        // 解包
        const char* packet = res.second->data;
        const BackendMsg* backMsg = (const BackendMsg*)packet;
        ret.second = backMsg->header.result;
        rspMsg.ParseFromString(backMsg->data);
    }

    return ret;
}

std::pair<int, uint32_t> sendMsg(uint32_t type, uint64_t targetId, const google::protobuf::Message& reqMsg, uint32_t reqCmdId, google::protobuf::Message& rspMsg, int timeOut) {
    World w;
    w.world = gSelf.world;
    w.type = type;
    w.id = 0;
    return sendMsg(w, targetId, reqMsg, reqCmdId, rspMsg, timeOut);
}

}

#endif