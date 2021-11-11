/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

/*
 * zeromq消息队列通信单元, 用户后端进程之间的通信
*/

#pragma once

#include <string>
#include <stdint.h>

// 使用的是router--dealer模式
class ZeromqUnit {
public:
    struct Context {
        Context();
        ~Context();
        void* m_ctx;
    };

    // addr格式：tcp://192.168.0.81:7000
    ZeromqUnit(uint32_t id, const std::string& addr);

    ~ZeromqUnit();

    bool listen();

    bool connect();

    int recvFd();

    void* getSock();

    bool isPollIn();

    int recvData(std::string& data);

    // @id表示发送到哪个dealer
    int sendData(uint32_t id, const std::string& data);

    int sendData(const std::string& data);

    uint32_t id();
protected:
    bool reInit();

    void printError();
private:
    static Context m_ctx;
    void* m_sock;
    std::string m_addr;
    uint32_t m_id;
    bool m_is_conn;
};



