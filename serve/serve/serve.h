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

#include "serve/serve/base.h"
#include <functional>

// 服务基类
class Serve {
public:
    virtual ~Serve();

    bool init(int argc, char** argv);

    void start();

    void useRouter(bool u);

    void stop();

    void onInit(std::function<bool(int, char**)> init);

    void onLoop(std::function<bool()> loop);

    void idleCount(uint32_t cnt);

    void sleep(uint32_t s);

    void useAsync(bool u);

    const World& self();

    void setLogFunc(std::function<void(const char*)> cb);
protected:
    bool parseArgv(int argc, char** argv);

protected:
    bool mStopped = false;
    uint32_t mIdleCnt = 200;
    uint32_t mSleep = 15;

    bool mUseRouter = false;
    bool mUseDealer = false;
    bool mUseAsyn = true;

    std::function<bool(int, char**)> mOnInit;
    std::function<bool()> mOnLoop;

    std::string mConfFile;
    World mSelf;
    LinkInfo mZmListen;
    LinkInfo mZmConnect;
    LinkInfo mNetListen;
    LinkInfo mNetConnect;
    LinkInfo mHttpListen;
    LinkInfo mRoutes;
};
