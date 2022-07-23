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
        uint32_t identify() const;
    };

    std::vector<Item> itemVec;
    std::map<World, Item> itemMap;    
};

// 服务基类
class Serve {
public:
#ifdef USE_IPC
    struct CommonRouter {
        virtual ~CommonRouter() {}

        World* getWorld(uint64_t id);

        uint64_t getId(const World& w);

        void setIdWorld(uint64_t id, const World& w);

        std::map<World, uint64_t> worldMap;
        std::map<uint64_t, World> idMap;
    };

    class RouterHandler : public ZeromqRouterHandler, public CommonRouter {
    protected:
        void onData(uint32_t uniqueId, uint32_t identify, const std::string& data);
    };

    class DealerHandler : public ZeromqDealerHandler, public CommonRouter {
    protected:
        void onData(uint32_t uniqueId, uint32_t identify, const std::string& data);
    };
#endif

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
    RouterHandler* mRouterHandler = 0;
    DealerHandler* mDealerHandler = 0;
#endif


};
