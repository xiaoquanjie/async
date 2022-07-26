/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include <string>
#include <map>
#include <vector>

// 这三个值，每个都不能大于255
struct World {
    uint32_t world = 0; 
    uint32_t type = 0;  // 可用来对应服务类型
    uint32_t id = 0;    // 可用来对应服务id

    World() {}
    World(uint32_t identify);
    bool operator <(const World&) const;
    uint32_t identify() const;
};

struct LinkInfo {
    struct Item {
        uint64_t uniqueId = 0;
        World w;
        std::string protocol;
        std::string ip;
        uint32_t port = 0;
        bool isTcp() const;
        bool isUdp() const;
        std::string addr() const;
    };

    std::vector<Item> itemVec; 
};

struct BackendHeader {
    static uint32_t size();
    uint32_t srcWorldId = 0;     // 消息从哪来
    uint32_t dstWorldId = 0;     // 消息到哪去
    uint64_t targetId = 0;       // 发给哪个目标
    uint32_t cmd = 0;            // 消息id
    uint64_t localFd = 0;        // 如果是zeromq,则fd表示为uniqueId。如果是net，则fd表示为fd
    uint64_t remoteFd = 0;
    uint64_t reqSeqId = 0;       // 请求的消息编号
    uint64_t rspSeqId = 0;       // 回复的消息编号
    uint32_t broadcast = 0;      // 是否广播，在没有路由的情况下，此字段不生效
    uint32_t result = 0;         // 消息返回时的结果
    uint32_t cmdLength = 0;      // 消息长度
};

struct BackendMsg {
    BackendHeader header;
    std::string data;
    void encode(std::string& output) const;
    void decode(const std::string& input);
};

struct HttpMsg {
    void* request;
    std::string m_url;
    std::string m_host;
    std::string m_query;
    std::string m_body;
};

void split(const std::string source, const std::string &separator, std::vector<std::string> &array);