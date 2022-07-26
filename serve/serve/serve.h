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

    bool mUseRouter = false;
    bool mUseDealer = false;
    std::string mConfFile;
    World mSelf;
    LinkInfo mZmListen;
    LinkInfo mZmConnect;
    LinkInfo mNetListen;
    LinkInfo mNetConnect;
    LinkInfo mHttpListen;
    LinkInfo mRoutes;
};
