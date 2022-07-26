/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#include "serve/serve/serve.h"
#include "common/transaction/transaction_mgr.h"
#include "serve/serve/serve_transaction.h"

// 定义服务类型
#define SRV_TYPE1 (1)
#define SRV_TYPE2 (2)

// 定义消息id
#define CmdEchoReq (1)
#define CmdEchoRsp (2)

uint32_t gSrvType = 0;

// 定义消息结构，可以直接使用protobuf
struct EchoReq {
    std::string data;

    // 实现两个接口
    bool ParseFromString(const std::string& data) {
        this->data = data;
        return true;
    }

    bool SerializeToString(std::string* data) const {
        *data = this->data;
        return true;
    }
};

struct EchoRsp {
    std::string data;

    // 实现两个接口
    bool ParseFromString(const std::string& data) {
        this->data = data;
        return true;
    }

    bool SerializeToString(std::string* data) const {
        *data = this->data;
        return true;
    }
};

// 实现一个接受EchoReq消息的trans
class EchoTransaction : public ServeTransaction<EchoReq, EchoRsp> {
public:
    int OnRequest() {
        printf("msg: %s\n", m_request.data.c_str());
        m_respond.data = "hello this echo reply";
        return 1;
    }
};

// 注册
REGIST_TRANSACTION_2(Echo);

// 实现一个tick
class TickTransaction : public BaseTickTransaction {
public:
    void OnTick(uint32_t curTime) {
        static bool s = false;
        if (!s) {
            s = true;
            // type为1给type为2的进程消息
            if (gSrvType == SRV_TYPE1) {
                EchoReq req;
                req.data = "hello, calling echo";
                EchoRsp rsp;

                {
                    // 发送请求
                    auto ret = backend::sendMsg(SRV_TYPE2, 1, req, CmdEchoReq, rsp);
                    if (ret.first == 0) {
                        // 成功
                        printf("successfully send msg:%d|%s\n", ret.second, rsp.data.c_str());
                    }
                    else {
                        // 失败
                        printf("failed to send msg:%d\n", ret.first);
                    }
                }
                {
                    // 广播
                    backend::broadcast(SRV_TYPE2, 1, req, CmdEchoReq);
                }


            }
        }
    }
};

// 注册
REGIST_TICK_TRANSACTION(TickTransaction);


int main(int argc, char** argv) {
    Serve srv;

    // 设置日志输出接口
    srv.setLogFunc([](const char* d) {
        printf("[yo] %s\n", d);
    });

    if (!srv.init(argc, argv)) {
        return -1;
    }

    gSrvType = srv.self().type;
    srv.start();
    return 0;
}