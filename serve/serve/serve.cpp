/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#include "serve/serve/serve.h"
#include "serve/serve/getopt.h"
#include "serve/serve/json.hpp"
#include "serve/serve/router.h"
#include "serve/serve/backend.h"
#include "serve/serve/http.h"
#include "serve/serve/gate.h"
#include "common/co_async/async.h"
#include "common/async/async.h"
#include "common/transaction/transaction_mgr.h"
#include "common/ipc/ipc.h"
#include "common/log.h"
#include "common/signal/sig.h"
#include <stdio.h>
#include <fstream>
#include <thread>

Serve::~Serve() {
}

bool Serve::init(int argc, char** argv) {
    do {
        if (!parseArgv(argc, argv)) {
            break;
        }
        if (mOnInit && !mOnInit(argc, argv)) {
            log("failed to onInit");
            break;
        }
    
#ifdef USE_IPC
        // 启动路由
        if (mUseRouter && !router::init(mZmListen, mRoutes)) {
            log("failed to router::init");
            return false;
        }
        if (mUseDealer && !backend::init(mSelf, mZmConnect)) {
            log("failed to backend::init");
            return false;
        }
#endif

#ifdef USE_NET
        if (mUseHttp && !http::init(0, mHttpListen)) {
            log("failed to http::init");
            return false;
        }
        if (mUseGate && !gate::init(0, mSelf, mNetListen)) {
            log("failed to gate::init");
            return false;
        }
        sig::initSignal(0);
        sig::regKill([this](uint32_t) {
            this->mStopped = true;
        });
#endif
        if (!mNetConnect.itemVec.empty()) {

        }
        
        return true;
    } while (false);

    return false;
}

void Serve::start() {
    bool isIdle = true;
    uint32_t idleCnt = 0;
    time_t now = 0;
    time_t tick = 0;

    while (!mStopped) {
        time(&now);
        isIdle = true;

#ifdef USE_IPC
        if (mUseRouter && router::update(now)) {
            isIdle = false;
        }
        if (mUseDealer && backend::update(now)) {
            isIdle = false;
        }
#endif
#ifdef USE_NET
        if (mUseHttp) {
            http::update(now);
        }
        if (mUseGate) {
            gate::update(now);
        }
        sig::update();
#endif
        if (mUseAsyn && co_async::loop(now)) {
            isIdle = false;
        }
        if (mOnLoop && mOnLoop()) {
            isIdle = false;
        }
        if ((now - tick) >= 1) {
            tick = now;
#ifdef USE_TRANS
            trans_mgr::tick(now);
#endif
        }

        if (isIdle) {
            idleCnt++;
        }
        else {
            idleCnt = 0;
        }

        if (idleCnt >= mIdleCnt) {
            idleCnt = 0;
            std::this_thread::sleep_for(std::chrono::milliseconds(mSleep));
        }
    }

    log("program exit");
}

void Serve::useRouter(bool u) {
    mUseRouter = u;
}

void Serve::stop() {
    mStopped = true;
}

void Serve::onInit(std::function<bool(int, char**)> init) {
    mOnInit = init;
}

void Serve::onLoop(std::function<bool()> loop) {
    mOnLoop = loop;
}

void Serve::idleCount(uint32_t cnt) {
    mIdleCnt = cnt;
}

void Serve::sleep(uint32_t s) {
    mSleep = s;
}

void Serve::useAsync(bool u) {
    mUseAsyn = u;
}

void Serve::useGate(bool u) {
    mUseGate = u;
}

const World& Serve::self() {
    return mSelf;
}

void Serve::setLogFunc(std::function<void(const char*)> cb) {
    setSafeLogFunc(cb);
}

bool Serve::parseArgv(int argc, char** argv) {
    struct option opt[] = {
        {"help", 0, NULL, 'h'},
        {"conf", 1, NULL, 'c'}
    };
    const char* descr[] = {
        "print command line options (currently set)",
        "specified config file. --conf filename"
    };

    auto usage = [&opt, &descr]() {
        log("DESCRIPTION");
        for (size_t idx = 0; idx < sizeof(opt) / sizeof(struct option); ++idx) {
            if (opt[idx].val == 0) {
                continue;
            }
            log("        -%c, --%s       %s", opt[idx].val, opt[idx].name, descr[idx]);
        }
    };

    while (true) {
        int optIdx = 0;
        int c = getopt_long(argc, argv, ":HhCc:Rr", opt, &optIdx);
        if (c == -1) {
            break;
        }

        switch (c)
        {
        case 'H':
        case 'h':
            usage();
            return false;
        
        case 'C':
        case 'c':
            if (optarg) {
                mConfFile = optarg;
            }
            break;

        case 'R':
        case 'r':
            // 启动路由功能
            mUseRouter = true;
            break;

        case '?':
            break;

        case ':':
            log("miss param");
            usage();
            return false;

        default:
            break;
        }
    }

    if (mConfFile.empty()) {
        log("need '-c config'");
        return false;
    }

    auto parseWorld = [](const nlohmann::json& j, World& self) {
        auto iter = j.find("world");
        if (iter != j.end()) {
            self.world = iter->get<uint32_t>();
        }

        iter = j.find("type");
        if (iter != j.end()) {
            self.type = iter->get<uint32_t>();
        }

        iter = j.find("id");
        if (iter != j.end()) {
            self.id = iter->get<uint32_t>();
        }
    };

    auto parseLinkInfo = [parseWorld](const nlohmann::json& j, LinkInfo& info) {
        if (!j.is_array()) {
            return;
        }

        for (size_t i = 0; i < j.size(); ++i) {
            const auto& subJ = j[i];
            LinkInfo::Item item;

            parseWorld(subJ, item.w);

            auto iter = subJ.find("protocol");
            if (iter != subJ.end()) {
                item.protocol = iter->get<std::string>();
            }

            iter = subJ.find("ip");
            if (iter != subJ.end()) {
                item.ip = iter->get<std::string>();
            }

            iter = subJ.find("port");
            if (iter != subJ.end()) {
                item.port = iter->get<uint32_t>();
            }

            info.itemVec.push_back(item);
        }
    };

    try {
        // parse server.json
        std::ifstream ifs(mConfFile);
        auto serveJson = nlohmann::json::parse(ifs, nullptr);

        // parse zm_listen
        parseWorld(serveJson["self"], mSelf);
        parseLinkInfo(serveJson["zm_listen"], mZmListen);
        parseLinkInfo(serveJson["zm_connect"], mZmConnect);
        parseLinkInfo(serveJson["net_listen"], mNetListen);
        parseLinkInfo(serveJson["net_connect"], mNetConnect);
        parseLinkInfo(serveJson["http_listen"], mHttpListen);
        parseLinkInfo(serveJson["routes"], mRoutes);

        if (!mZmConnect.itemVec.empty()) {
            mUseDealer = true;
        }
        if (!mHttpListen.itemVec.empty()) {
            mUseHttp = true;
        }
    }
    catch(nlohmann::detail::exception& e) {
        log("failed to parse %s|%s", mConfFile.c_str(), e.what());
        return false;
    }

    auto worldCheck = [](const World& w, int type)-> bool {
        if (type & 1) {
            if (w.world == 0 || w.world > 255) {
                return false;
            }
        }
        if (type & 2) {
            if (w.type == 0 || w.type > 255) {
                return false;
            }
        }
        if (type & 4) {
            if (w.id == 0 || w.id > 255) {
                return false;
            }
        }
        return true;
    };

    // 检查配置
    if (mUseRouter) {
        for (auto item : mZmListen.itemVec) {
            if (!worldCheck(item.w, 1)) {
                log("conf: zm_listen.world error");
                return false;
            }
        }
        for (auto item : mRoutes.itemVec) {
            if (!worldCheck(item.w, 1 | 2 | 4)) {
                log("conf: routes error");
                return false;
            }
        }
    }

    if (mUseDealer && !worldCheck(mSelf, 1 | 2 | 4)) {
        log("conf: self error");
        // self不能为空
        return false;
    }

    if (mUseHttp) {
        for (auto item : mHttpListen.itemVec) {
            if (!worldCheck(item.w, 4)) {
                log("conf: http_listen.id error");
                return false;
            }
        }
    }

    for (auto item : mNetListen.itemVec) {
        if (item.protocol != "tcp" && item.protocol != "udp") {
            log("conf: net_listen.protocol error");
            return false;
        }
    }
    
    return true;
}

///////////////////////////////////////////////////////////////
