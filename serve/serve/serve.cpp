#include "serve/serve/serve.h"
#include "serve/serve/getopt.h"
#include "serve/serve/json.hpp"
#include "serve/serve/router.h"
#include "serve/serve/backend.h"
#include <stdio.h>
#include <fstream>
#include <thread>

Serve::~Serve() {
}

bool Serve::Init(int argc, char** argv) {
    do {
        if (!parseArgv(argc, argv)) {
            break;
        }
        if (!onInit(argc, argv)) {
            printf("failed to onInit\n");
            break;
        }

        // 启动路由
        if (mUseRouter) {
            router::init(mZmListen, mRoutes);
        }
        if (mUseDealer) {
            backend::init(mSelf, mZmConnect);
        }

        if (!mNetListen.itemVec.empty()) {

        }
        if (!mNetConnect.itemVec.empty()) {

        }
        if (!mHttpListen.itemVec.empty()) {

        }

        return true;
    } while (false);

    return false;
}

void Serve::Start() {
    bool isIdle = true;
    uint32_t idleCnt = 0;
    time_t now = 0;

    while (!mStopped) {
        time(&now);
        isIdle = true;

        if (mUseRouter) {
            if (router::update(now)) {
                isIdle = false;
            }
        }
        if (mUseDealer) {
            if (backend::update(now)) {
                isIdle = false;
            }
        }

        if (onLoop()) {
            isIdle = false;
        }

        if (isIdle) {
            idleCnt++;
        }
        else {
            idleCnt = 0;
        }

        if (idleCnt >= 200) {
            idleCnt = 0;
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
        }
    }
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
        printf("DESCRIPTION\n");
        for (size_t idx = 0; idx < sizeof(opt) / sizeof(struct option); ++idx) {
            if (opt[idx].val == 0) {
                continue;
            }
            printf("        -%c, --%s       %s\n", opt[idx].val, opt[idx].name, descr[idx]);
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
            printf("miss param\n");
            usage();
            return false;

        default:
            break;
        }
    }

    if (mConfFile.empty()) {
        printf("need '-c config'\n");
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
    }
    catch(nlohmann::detail::exception& e) {
        printf("failed to parse %s|%s\n", mConfFile.c_str(), e.what());
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
                return false;
            }
        }
        for (auto item : mRoutes.itemVec) {
            if (!worldCheck(item.w, 1 | 2 | 4)) {
                return false;
            }
        }
    }

    if (mUseDealer) {
        // self不能为空
        if (!worldCheck(mSelf, 1 | 2 | 4)) {
            return false;
        }
    }
    
    return true;
}

///////////////////////////////////////////////////////////////

#ifdef TEST_MAIN

int main(int argc, char** argv) {
    Serve srv;
    if (!srv.Init(argc, argv)) {
        return -1;
    }

    srv.Start();
    return 0;
}

#endif