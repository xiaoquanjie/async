/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include "serve/serve/base.h"
#include "common/co_async/ipc/co_ipc.h"

namespace backend {

bool init(const World& self, const LinkInfo& link);

bool update(time_t now);

// 添加消息中间件
void use(std::function<bool(uint64_t, uint32_t, const std::string&)> fn);

uint32_t selfWorld();

uint32_t selfWorldId();

void send(BackendMsg& frame);

template<typename MsgType>
void notifyBack(uint32_t worldId, uint64_t targetId, const MsgType& msg, uint32_t cmdId, uint32_t frontSeqNo, uint64_t rspSeqId, uint32_t result, uint32_t broadcast) {
    BackendMsg frame;
    frame.header.srcWorldId = selfWorldId();
    frame.header.dstWorldId = worldId;
    frame.header.targetId = targetId;
    frame.header.cmd = cmdId;
    frame.header.frontSeqNo = frontSeqNo;
    frame.header.rspSeqId = rspSeqId;
    frame.header.result = result;
    frame.header.broadcast = broadcast;

    // 序列化msg
    msg.SerializeToString(&frame.data);
    frame.header.cmdLength = frame.data.size();

    // 发送
    send(frame);
}

/*
 * w: 发给哪个world进程
   targetId: 发给谁
   msg: 消息结构
   cmdId：消息id
*/
template<typename MsgType>
void notifyMsg(World w, uint64_t targetId, const MsgType& msg, uint32_t cmdId) {
    uint32_t dstWorldId = w.identify();
    notifyBack(dstWorldId, targetId, msg, cmdId, 0, 0, 0, 0);
}

/*
 * type发到哪个type上，如果有多个type实例，则由路由来决定具体的实例
*/
template<typename MsgType>
void notifyMsg(uint32_t type, uint64_t targetId, const MsgType& msg, uint32_t cmdId) {
    World w;
    w.world = selfWorld(); // 默认是同一个世界的
    w.type = type;
    w.id = 0;
    notifyMsg(w, targetId, msg, cmdId);
}

/*
 * 广播
*/
template<typename MsgType>
void broadcast(uint32_t type, uint64_t targetId, const MsgType& msg, uint32_t cmdId) {
    World w;
    w.world = selfWorld(); // 默认是同一个世界的
    w.type = type;
    w.id = 0;
    notifyBack(w.identify(), targetId, msg, cmdId, 0, 0, 0, 1);
}

/*
 * return: co_async::E_OK/E_TIMEOUT/E_ERROR
   w: 发给哪个world进程
   targetId: 发给谁
   reqMsg: 请求的消息结构
   reqCmdId：消息id
   rspMsg: 回复的消息结构
   timeOut: 超时时间
*/
template<typename ReqMsgType, typename RspMsgType>
std::pair<int, uint32_t> sendMsg(World w, uint64_t targetId, const ReqMsgType& reqMsg, uint32_t reqCmdId, RspMsgType& rspMsg, int timeOut = 5 * 1000) {
    BackendMsg frame;
    frame.header.srcWorldId = selfWorldId(); // 默认是同一个世界的
    frame.header.dstWorldId = w.identify();
    frame.header.targetId = targetId;
    frame.header.cmd = reqCmdId;

    // 序列化msg
    reqMsg.SerializeToString(&frame.data);
    frame.header.cmdLength = frame.data.size();

    auto res = co_async::ipc::send([&frame](uint64_t seqId) {
        frame.header.reqSeqId = seqId;
        send(frame);
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

/*
 * type发到哪个type上，如果有多个type实例，则由路由来决定具体的实例
*/
template<typename ReqMsgType, typename RspMsgType>
std::pair<int, uint32_t> sendMsg(uint32_t type, uint64_t targetId, const ReqMsgType& reqMsg, uint32_t reqCmdId, RspMsgType& rspMsg, int timeOut = 5 * 1000) {
    World w;
    w.world = selfWorld(); // 默认是同一个世界的
    w.type = type;
    w.id = 0;
    return sendMsg(w, targetId, reqMsg, reqCmdId, rspMsg, timeOut);
}

}