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
        uint32_t m_gen_id;
        void* m_ctx;
    };

    // addr格式：tcp://192.168.0.81:7000
    ZeromqUnit(uint32_t identify, const std::string& addr);

    virtual ~ZeromqUnit();

    int recvFd();

    void* getSock();

    bool isPollIn();

    virtual int recvData(std::string& identify, std::string& data) = 0;

    virtual std::string identify() = 0;

    uint32_t zeromqId();

    uint32_t uniqueId();
protected:
    bool reInit(bool is_router);

    int send(bool is_router, const std::string& identify, const std::string& data);

    void printError();

protected:
    static Context m_ctx;
    void* m_sock;
    std::string m_addr;
    uint32_t m_unique_id;   // 内部分配的唯一id
    uint32_t m_zeromq_id;   // zeromq用的标识符id
};

/////////////////////////////////////////////////////

// router
class ZeromqRouter : public ZeromqUnit {
public:
    ZeromqRouter(uint32_t identify, const std::string& addr);

    bool listen();

    virtual int recvData(std::string& identify, std::string& data) override;

    virtual std::string identify() override;

    // @id表示发送到哪个dealer
    int sendData(uint32_t identify, const std::string& data);

    // @identify表示发送到哪个标识符
    int sendData(const std::string& identify, const std::string& data);
};

////////////////////////////////////////////////////

// dealer
class ZeromqDealer : public ZeromqUnit {
public:
    ZeromqDealer(uint32_t identify, const std::string& addr);    

    bool connect();

    virtual int recvData(std::string& identify, std::string& data) override;

    virtual std::string identify() override;

    int sendData(const std::string& data);
};