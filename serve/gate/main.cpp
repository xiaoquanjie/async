/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

/*
 * 提供一个gate服模板
 * 可以使用默认的前端消息格式，也可以自定义消息格式
 * 主要实现的接口如下：
 *  1、gate::setHeader
 *  2、gate::setCmdParser
 *  3、gate::setPacket
 *  4、backend::use
*/

#include "serve/serve/serve.h"
#include "serve/serve/gate.h"
#include "serve/serve/backend.h"
#include "async/log.h"

int main(int argc, char** argv) {
    Serve srv;
    // 启用网关功能
    srv.useGate(true);
    if (!srv.init(argc, argv)) {
        return -1;
    }

    // 设置默认的前端协议解析方法
    uint32_t headerSize = sizeof(FrontendHeader);
    gate::setHeader(headerSize, [](const char* d, uint32_t)-> uint32_t {
        const FrontendHeader* h = (const FrontendHeader*)d;
        FrontendHeader h2;
        h2.decode(*h);
        return h2.cmdLength;
    });

    // 设置消息速率, 一秒最多收50条消息，过多则丢弃
    gate::setMsgFreq(50);

    // 设置空消息连接超时时长, 60秒
    gate::setExpire(60);

    // 设置cmd身份识别接口
    gate::setCmdParser([](void* msg, uint32_t type) -> CmdInfo {
        CmdInfo info;
        if (type == 1) {
            FrontendMsg* fMsg = (FrontendMsg*)msg;
            // Todo: 根据消息设置info的各个字段
        }
        else if (type == 2) {
            BackendMsg* bMsg = (BackendMsg*)msg;
            // Todo: 根据消息设置info的各个字段
        }

        return info;
    });

    // 设置默认的包处理接口
    gate::setPacket([](uint32_t fd, const char* d, uint32_t len) {
        // 1、使用默认的消息格式进行解包
        FrontendMsg frontMsg; 
        frontMsg.decode(d, len);

        uint64_t targetId = 0;

        // 2、获取身份信息
        CmdInfo cmdInfo = gate::getCmdParser()(&frontMsg, 1);

        // 3、判断是否为登录消息，第一条消息必须为登录消息
        if (!gate::getAuth(fd)) {
            if (!cmdInfo.isLogin) {
                // 非登录消息不允许发
                log("[gate] not allowed, not auth, fd:%d, cmd:%d", fd, frontMsg.header.cmd);
                return;
            }
            targetId = cmdInfo.targetId;
        }
        else {
            targetId = gate::getTargetId(fd);
        }

        if (targetId == 0) {
            log("[gate] invalid target id");
            return;
        }

        // 4、将消息转为后端格式
        BackendMsg backMsg;
        backMsg.header.targetId = targetId;
        backMsg.header.srcWorldId = gate::selfWorldId();
        backMsg.header.dstWorldId = cmdInfo.w.identify();
        backMsg.header.cmd = frontMsg.header.cmd;
        backMsg.header.frontSeqNo = frontMsg.header.frontSeqNo;
        backMsg.header.remoteFd = fd;
        backMsg.data = frontMsg.data;

        // 5、将消息转发到后端
        backend::send(backMsg);
    });

    // backend设置中间件
    backend::use([](uint64_t uniqueId, uint32_t identify, const std::string& data)-> bool {
        // 1、解析消息
        BackendMsg backMsg;
        backMsg.decode(data);

        // 如果frontSeqNo不为0，则表示是前端的数据包，否则是后端的消息
        if (backMsg.header.frontSeqNo == 0) {
            // 转到默认处理，即transaction层处理
            return false;
        }

        // 将消息转为前端格式
        FrontendMsg frontMsg; 
        frontMsg.header.cmd = backMsg.header.cmd;
        frontMsg.header.frontSeqNo = backMsg.header.frontSeqNo;
        frontMsg.header.result = backMsg.header.result;
        frontMsg.header.cmdLength = backMsg.data.size();
        frontMsg.data = backMsg.data;

        uint64_t targetId = 0;
        uint32_t fd = 0;

        // 2、获取身份信息
        CmdInfo cmdInfo = gate::getCmdParser()(&backMsg, 2);
        if (cmdInfo.isLogin) {
            // Todo: 从消息中取出targetId和fd, 一般建议在登录消息中带着uid和fd 
            targetId = cmdInfo.targetId;
            fd = cmdInfo.fd;
        }
        else {
            targetId = backMsg.header.targetId;
            fd = gate::getFd(targetId);
        }

        if (fd != 0) {
            // 转发到前端
            std::string output;
            frontMsg.encode(output);
            gate::send(fd, output.c_str(), output.size());

            // 如果还没鉴权
            if (!gate::getAuth(fd)) {
                // 消息成功
                if (backMsg.header.result == 0) {
                    // 设置完成完成鉴权
                    gate::setTargetId(fd, targetId);
                    gate::setAuth(fd, true);
                }
                // 消息失败
                else {
                    // 关闭连接
                    gate::close(fd);
                }
            }
        }
        else {
            log("[gate] invalid fd, target:%ld", targetId);
        }

        // 不传送到下一层
        return true;
    });

    srv.start();
    return 0;
}