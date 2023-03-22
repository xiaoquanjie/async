/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

/*
 * 提供以下网关功能
   1、允许指定前端协议
   2、将前端协议转换为后端协议，并转发到后端
   3、验证fd的合法性
   4、限制fd的消息速率
*/

#pragma once

#include "serve/serve/base.h"
#include <functional>

namespace gate {

bool init(void* event_base, const World& self, const LinkInfo& link);

bool update(time_t now);

// 指定前端协议解析方法
// 设置包解析器:
// @headerSize: 头包长度
// @parser: 返回值包体长度
void setHeader(uint32_t headerSize, std::function<uint32_t(const char* d, uint32_t len)> parser);

// 设置包处理接口
void setPacket(std::function<void(uint32_t fd, const char* d, uint32_t len)> packet);

// 设置cmd身份识别接口
void setCmdParser(std::function<CmdInfo(void*, uint32_t)> parser);

std::function<CmdInfo(void*, uint32_t)> getCmdParser();

// 设置消息速率
void setMsgFreq(uint32_t f);

// 设置是否授权
void setAuth(uint32_t fd, bool flag);

bool getAuth(uint32_t fd);

// 设置目标id, 这个会导致原来的连接失效
void setTargetId(uint32_t fd, uint64_t targetId);

uint64_t getTargetId(uint32_t fd);

uint32_t getFd(uint64_t targetId);

// 心跳超时，即多过长时间没有数据包就断开连接
void setExpire(uint32_t t);

void close(uint32_t fd);

int send(uint32_t fd, const char* buf, uint32_t len);

uint32_t selfWorldId();

}