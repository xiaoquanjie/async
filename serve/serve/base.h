/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include <string>
#include <unordered_map>
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
        std::string rawAddr() const;
    };

    std::vector<Item> itemVec; 
};

struct BackendHeader {
    static uint32_t size();
    uint32_t srcWorldId = 0;     // 消息从哪来
    uint32_t dstWorldId = 0;     // 消息到哪去
    uint64_t targetId = 0;       // 发给哪个目标
    uint32_t cmd = 0;            // 消息id
    uint64_t remoteFd = 0;        // 如果是zeromq,则fd表示为uniqueId。如果是net，则fd表示为fd
    uint64_t reqSeqId = 0;       // 请求的消息编号
    uint64_t rspSeqId = 0;       // 回复的消息编号
    uint32_t broadcast = 0;      // 是否广播，在没有路由的情况下，此字段不生效
    uint32_t result = 0;         // 消息返回时的结果
    uint32_t frontSeqNo = 0;     // 前端请求编号
    uint32_t cmdLength = 0;      // 消息长度，不包括包头
};

struct BackendMsg {
    BackendHeader header;
    std::string data;
    void encode(std::string& output) const;
    void decode(const std::string& input);
};

// 默认的前端协议, 里面的字段全使用网络字节序
struct FrontendHeader {
    static uint32_t size();
    void encode(FrontendHeader& h) const;
    void decode(const FrontendHeader& h);
    uint32_t cmdLength = 0;      // 消息长度，不包括包头
    uint32_t frontSeqNo = 0;     // 前端请求编号
    uint32_t cmd = 0;            // 消息id
    uint32_t result = 0;         // 消息返回时的结果
};

struct FrontendMsg {
    FrontendHeader header;
    std::string data;
    void encode(std::string& output) const;
    void decode(const std::string& input);
    void decode(const char* d, uint32_t len);
};

struct HttpRequest {
    void* r;
    std::string url;
    std::string host;
    std::string squery;
    std::string body;
    std::unordered_map<std::string, std::string> query;

    void swap(HttpRequest& req);
};

struct HttpRespond {
    HttpRespond() {
        header["Content-Type"] = "text/plain";
    }
    std::string body;
    std::unordered_map<std::string, std::string> header;

    void swap(HttpRespond& rsp);
};

struct HttpMsg {
    HttpRequest req;
    HttpRespond rsp;
};

struct CmdInfo {
    World w;                // 表明消息服务身份
    uint64_t targetId = 0;  // 
    uint32_t fd = 0;
    bool isLogin = false;   // 是否为登录消息
};

void split(const std::string source, const std::string &separator, std::vector<std::string> &array);