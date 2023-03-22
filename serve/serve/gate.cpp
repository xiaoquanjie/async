/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_NET 

#include "serve/serve/gate.h"
#include "common/net/tcp_listener.h"
#include "common/log.h"
#include <memory>
#include <unordered_map>

namespace gate {

LinkInfo gLinkInfo;
World gSelf;
uint32_t gMsgFreq = 100;
uint32_t gExpire = 5 * 60;
uint32_t gExpireCheck = 0;

typedef std::shared_ptr<net::TcpListener> TcpListenerPtr;
TcpListenerPtr gListener = nullptr;

struct TargetIdInfo {
    uint32_t fd = 0;
    uint64_t targetId = 0;
    std::string ip;
    bool isAuth = false;    // 是否已授权的标记
    time_t connTime = 0;  // 连接的时间
    time_t msgTime = 0;   // 最近一条消息的时间
    uint32_t msgFreq = 0;   // 消息频率
};

// key: targetId, value: fd
std::unordered_map<uint64_t, uint32_t> gTargetFdMap;
// key: fd, value: TargetIdInfo
std::unordered_map<uint32_t, TargetIdInfo> gFdMap;

// 包处理接口
std::function<void(uint32_t fd, const char* d, uint32_t len)> gPacket;
std::function<CmdInfo(void*, uint32_t)> gCmdParser;

void onAcceptCb(net::TcpListener*, uint32_t fd) {
    gFdMap.erase(fd);
    auto& info = gFdMap[fd];
    time(&info.connTime);
    info.ip = gListener->getIp(fd);
    info.msgTime = info.connTime;
    log("[gate] a accepted connection: %d, ip:%s, t:%d", fd, info.ip.c_str(), info.connTime);
}

void onClosedCb(net::TcpListener*, uint32_t fd) {
    auto iter = gFdMap.find(fd);
    if (iter == gFdMap.end()) {
        return;
    }

    auto targetId = iter->second.targetId;
    auto iter2 = gTargetFdMap.find(targetId);
    if (iter2 != gTargetFdMap.end() && iter2->second == fd) {
        gTargetFdMap.erase(iter2);
    }

    gFdMap.erase(iter);
    log("[gate] a closed connection: %d, targetId:%llu", fd, targetId);
}

void onDataCb(net::TcpListener*, uint32_t fd, const char* d, uint32_t len) {
    // 控制fd的消息速率
    auto iter = gFdMap.find(fd);
    if (iter == gFdMap.end()) {
        log("[gate] connection is closed: %d", fd);
        return;
    }

    time_t curTime = 0;
    time(&curTime);
    if (iter->second.msgTime != curTime) {
        iter->second.msgTime = curTime;
        iter->second.msgFreq = 1;
    }
    else {
        iter->second.msgFreq++;
    }

    if (iter->second.msgFreq > gMsgFreq) {
        // 消息过多，丢掉并给出日志
        log("[gate] msg frequency over limited: %d, targetId: %llu, freq: %d", fd, iter->second.targetId, iter->second.msgFreq);
        return;
    }

    if (gPacket) {
        gPacket(fd, d, len);
    }
}

// 只会监听一个
bool init(void* event_base, const World& self, const LinkInfo& link) {
    gSelf = self;
    gLinkInfo = link;

    for (auto item : gLinkInfo.itemVec) {
        if (!item.isTcp()) {
            continue;
        }

        gListener = std::make_shared<net::TcpListener>(event_base);
        return gListener->listen(item.rawAddr());
    }

    return false;
}

bool update(time_t now) {
    if (gListener) {
        gListener->update(now);
    }

    if (now - gExpireCheck >= 15) {
        gExpireCheck = now;
        for (auto& item : gFdMap) {
            if (now - item.second.msgTime >= gExpire) {
                // 关闭连接
                close(item.first);
            }
        }
    }

    return true;
}

void setHeader(uint32_t headerSize, std::function<uint32_t(const char* d, uint32_t len)> parser) {
    if (!gListener) {
        return;
    }

    gListener->setParser(headerSize, parser);
    gListener->setAcceptCb(onAcceptCb);
    gListener->setClosedCb(onClosedCb);
    gListener->setDataCb(onDataCb);
}

void setPacket(std::function<void(uint32_t fd, const char* d, uint32_t len)> packet) {
    gPacket = packet;
}

void setCmdParser(std::function<CmdInfo(void*, uint32_t)> parser) {
    gCmdParser = parser;
}

std::function<CmdInfo(void*, uint32_t)> getCmdParser() {
    return gCmdParser;
}

void setMsgFreq(uint32_t f) {
    gMsgFreq = f;
}

void setAuth(uint32_t fd, bool flag) {
    auto iter = gFdMap.find(fd);
    if (iter == gFdMap.end()) {
        return;
    }

    iter->second.isAuth = flag;
}

bool getAuth(uint32_t fd) {
    auto iter = gFdMap.find(fd);
    if (iter == gFdMap.end()) {
        return false;
    }

    return iter->second.isAuth;
}

void setTargetId(uint32_t fd, uint64_t targetId) {
    auto iter = gFdMap.find(fd);
    if (iter == gFdMap.end()) {
        return;
    }

    iter->second.targetId = targetId;
    gTargetFdMap[targetId] = fd;
}

uint64_t getTargetId(uint32_t fd) {
    auto iter = gFdMap.find(fd);
    if (iter == gFdMap.end()) {
        return 0;
    }

    return iter->second.targetId;
}

uint32_t getFd(uint64_t targetId) {
    auto iter = gTargetFdMap.find(targetId);
    if (iter == gTargetFdMap.end()) {
        return 0;
    }

    return iter->second;
}

void setExpire(uint32_t t) {
    gExpire = t;
}

void close(uint32_t fd) {
    if (gListener) {
        gListener->close(fd);
    }
}

int send(uint32_t fd, const char* buf, uint32_t len) {
    if (gListener) {
        return gListener->send(fd, buf, len);
    }
    return 0;
}

uint32_t selfWorldId() {
    return gSelf.identify();
}


}

#endif