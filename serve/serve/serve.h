/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

/*
 * 提供类似的服务器进程脚手架
   设计原则：
        1、根据配置自动监听相应的类型端口
        2、根据配置自动的连接到相应类型地址，如果是zm_connect还会进行注册
        3、根据收到的消息启动已注册的transaction
        4、为了便于测试，serve允许同时能扮演router, serve A, serve B三者的角色，router为A,B之间的通信做转发
*/

#pragma once

#include <string>
#include <map>
#include <vector>

#ifdef USE_IPC
#include "common/ipc/zero_mq_handler.h"
#endif

// 这三个值，每个都不能大于255
struct World {
    uint32_t world = 0; 
    uint32_t type = 0;  // 可用来对应服务类型
    uint32_t id = 0;    // 可用来对应服务id

    bool operator <(const World&) const;
    uint32_t identify() const;
};

struct LinkInfo {
    struct Item {
        World w;
        std::string protocol;
        std::string ip;
        uint32_t port = 0;
        bool isTcp() const;
        bool isUdp() const;
        std::string addr() const;
    };

    std::vector<Item> itemVec;
    std::map<World, Item> itemMap;    
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

#ifdef USE_IPC
struct DefaultCommZq {
    virtual ~DefaultCommZq() {}
    World* getWorld(uint64_t id);
    uint64_t getId(const World& w);
    void setIdWorld(uint64_t id, const World& w);
    std::map<World, uint64_t> worldMap;
    std::map<uint64_t, World> idMap;
};

struct DefaultZqRouter : public ZeromqRouterHandler, public DefaultCommZq {
    void onData(uint64_t uniqueId, uint32_t identify, const std::string& data) override;
};

struct DefaultZqDealer : public ZeromqDealerHandler, public DefaultCommZq {
    void onData(uint64_t uniqueId, uint32_t identify, const std::string& data) override;
};

#endif

// 服务基类
class Serve {
public:
    virtual ~Serve();

    bool Init(int argc, char** argv);

    void Start();

protected:
    bool parseArgv(int argc, char** argv);

    virtual bool onInit(int argc, char** argv) { return true; }

    virtual bool onLoop() { return false; }

protected:
    bool mStopped = false;

    std::string mConfFile;
    LinkInfo mZmListen;
    LinkInfo mZmConnect;
    LinkInfo mNetListen;
    LinkInfo mNetConnect;
    LinkInfo mHttpListen;
    LinkInfo mRoutes;

#ifdef USE_IPC
    // 想重写DefaultZqRouter/DefaultZqDealer，可以在onInit接口里实现
    DefaultZqRouter* mZqRouter = 0;
    DefaultZqDealer* mZqDealer = 0;
#endif


};
