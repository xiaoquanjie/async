/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include "serve/serve/base.h"

namespace google {
	namespace protobuf {
		class Message;
	}
}

namespace backend {

bool init(const World& self, const LinkInfo& link);

bool update(time_t now);

void send(uint64_t uniqueId, const std::string& data);

void send(BackendMsg& frame);

/*
 * w: 发给哪个world进程
   targetId: 发给谁
   msg: 消息结构
   cmdId：消息id
*/
void notifyMsg(World w, uint64_t targetId, const google::protobuf::Message& msg, uint32_t cmdId);

/*
 * type发到哪个type上，如果有多个type实例，则由路由来决定具体的实例
*/
void notifyMsg(uint32_t type, uint64_t targetId, const google::protobuf::Message& msg, uint32_t cmdId);

void notifyBack(uint32_t worldId, uint64_t targetId, const google::protobuf::Message& msg, uint32_t cmdId, uint64_t rspSeqId, uint32_t result, uint32_t broadcast = 0);

/*
 * 广播
*/
void broadcast(uint32_t type, uint64_t targetId, const google::protobuf::Message& msg, uint32_t cmdId);

/*
 * return: co_async::E_OK/E_TIMEOUT/E_ERROR
   w: 发给哪个world进程
   targetId: 发给谁
   reqMsg: 请求的消息结构
   reqCmdId：消息id
   rspMsg: 回复的消息结构
   timeOut: 超时时间
*/
std::pair<int, uint32_t> sendMsg(World w, uint64_t targetId, const google::protobuf::Message& reqMsg, uint32_t reqCmdId, google::protobuf::Message& rspMsg, int timeOut = 5 * 1000);

/*
 * type发到哪个type上，如果有多个type实例，则由路由来决定具体的实例
*/
std::pair<int, uint32_t> sendMsg(uint32_t type, uint64_t targetId, const google::protobuf::Message& reqMsg, uint32_t reqCmdId, google::protobuf::Message& rspMsg, int timeOut = 5 * 1000);

}