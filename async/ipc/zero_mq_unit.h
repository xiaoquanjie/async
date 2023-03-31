/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

/*
 * zeromq消息队列通信单元, 用户后端进程之间的通信
*/

#pragma once

#include <string>
#include <stdint.h>

namespace ipc {

// 使用的是router--dealer模式
class ZeromqUnit {
public:
    struct Context {
        Context();
        ~Context();
        uint64_t m_gen_id;
        void* m_ctx;
    };

    // addr格式：tcp://192.168.0.81:7000
    ZeromqUnit(uint32_t identify, const std::string& addr);

    virtual ~ZeromqUnit();

    int recvFd();

    void* getSock();

    bool isPollIn();

    virtual int recvData(uint32_t& /*identify*/, std::string& /*data*/) = 0;

    uint64_t uniqueId();
protected:
    // 编码身份
    std::string encodeIdentify(bool isRouter);

    std::string encodeIdentify(bool isRouter, uint32_t id);

    // 解码身份
    uint32_t decodeIdentify(const std::string& id);

    bool reInit(bool is_router);

    int send(bool is_router, const std::string& identify, const std::string& data);

    void printError();

protected:
    static Context m_ctx;
    void* m_sock;
    std::string m_addr;
    uint64_t m_unique_id;   // 内部分配的唯一id
    uint32_t m_identify;   // zeromq用的标识符id
};

/////////////////////////////////////////////////////

// router
class ZeromqRouter : public ZeromqUnit {
public:
    ZeromqRouter(const std::string& addr);

    bool listen();

    virtual int recvData(uint32_t& identify, std::string& data) override;

    // @identify表示发送到哪个标识符
    int sendData(uint32_t identify, const std::string& data);
};

////////////////////////////////////////////////////

// dealer
class ZeromqDealer : public ZeromqUnit {
public:
    /* 
        @identify 是客户端的唯一id, 每个连接一个id（不管连接是否同进程），
        因为zeromq是消息队列，tcp连接断开后，数据并不会丢，所以它要求有个能证明之前链接的凭证，
    */
    ZeromqDealer(uint32_t identify, const std::string& addr);    

    bool connect();

    virtual int recvData(uint32_t&, std::string& data) override;

    int sendData(const std::string& data);
};

}