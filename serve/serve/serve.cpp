#include "serve/serve/serve.h"
#include "serve/serve/getopt.h"
#include "serve/serve/json.hpp"
#include <stdio.h>
#include <fstream>
#include <thread>

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#include <Winsock2.h>
#else
#include <arpa/inet.h>
#endif

bool World::operator <(const World& w) const {
    if (this->world < w.world) {
        return true;
    }
    if (this->world > w.world) {
        return false;
    }
    if (this->type < w.type) {
        return true;
    }
    if (this->type > w.type) {
        return false;
    }
    if (this->id < w.id) {
        return true;
    }
    if (this->id > w.id) {
        return false;
    }
    return false;
}

uint32_t World::identify() const {
    char ipBuf[50] = {0};
    sprintf(ipBuf, "%d.%d.%d.%d", 0, this->world, this->type, this->id);
    auto id = ntohl(inet_addr(ipBuf));
    return id;
}

bool LinkInfo::Item::isTcp() const {
    return (this->protocol == "tcp");
}

bool LinkInfo::Item::isUdp() const {
    return (this->protocol == "udp");
}

std::string LinkInfo::Item::addr() const {
    std::string addr = this->protocol + "://";
    addr += this->ip + ":";
    addr += std::to_string(this->port);
    return addr;
}

uint32_t BackendHeader::size() {
    static uint32_t s = sizeof(BackendHeader);
    return s;
}

void BackendMsg::encode(std::string& output) const {
    output.assign(reinterpret_cast<const char*>(&header), header.size());
	output.append(data);
}

void BackendMsg::decode(const std::string& input) {
    auto p = reinterpret_cast<const BackendHeader*>(input.c_str());
	this->header = *p;
	data.assign(input.c_str() + this->header.size(), input.size() - this->header.size());
}

#ifdef USE_IPC
World* DefaultCommZq::getWorld(uint64_t id) {
    auto iter = idMap.find(id);
    if (iter == idMap.end()) {
        return nullptr;
    }
    return &iter->second;
}

uint64_t DefaultCommZq::getId(const World& w) {
    auto iter = worldMap.find(w);
    if (iter == worldMap.end()) {
        return 0;
    }
    return iter->second;
}

void DefaultCommZq::setIdWorld(uint64_t id, const World& w) {
    worldMap[w] = id;
    idMap[id] = w;
}

void DefaultZqRouter::onData(uint64_t uniqueId, uint32_t identify, const std::string& data) {
    // 默认处理：解析出消息包，将消息头的数据填充好后调用trans_mgr::handle
}

void DefaultZqDealer::onData(uint64_t uniqueId, uint32_t, const std::string& data) {
    // 默认处理：解析出消息包，将消息头的数据填充好后调用trans_mgr::handle
    BackendMsg message;
    message.decode(data);
}
#endif

Serve::~Serve() {
#ifdef USE_IPC
    if (mZqRouter) {
        delete mZqRouter;
    }
    if (mZqDealer) {
        delete mZqDealer;
    }
#endif
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

#ifdef USE_IPC
        // 建立zm_listen
        if (!mZmListen.itemVec.empty()) {
            if (!mZqRouter) {
                mZqRouter = new DefaultZqRouter;
            }
            for (auto& item : mZmListen.itemVec) {
                uint64_t id = mZqRouter->listen(item.addr());
                mZqRouter->setIdWorld(id, item.w);
            }
        }
        if (!mZmConnect.itemVec.empty()) {
            if (!mZqDealer) {
                mZqDealer = new DefaultZqDealer;
            }
            for (auto& item : mZmConnect.itemVec) {
                uint64_t id = mZqDealer->connect(item.w.identify(), item.addr());
                mZqDealer->setIdWorld(id, item.w);
            }
        }
#endif

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

#ifdef USE_IPC
        if (mZqRouter) {
            if (mZqRouter->update(now)) {
                isIdle = false;
            }
        }
        if (mZqDealer) {
            if (mZqDealer->update(now)) {
                isIdle = false;
            }
        }
#endif

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
        int c = getopt_long(argc, argv, ":HhCc:", opt, &optIdx);
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

    auto parseLinkInfo = [](const nlohmann::json& j, LinkInfo& info, int type)-> bool {
        if (!j.is_array()) {
            return false;
        }

        for (size_t i = 0; i < j.size(); ++i) {
            const auto& subJ = j[i];
            LinkInfo::Item item;
            if (type & 1) {
                item.w.world = subJ["world"].get<uint32_t>();
                item.w.type = subJ["type"].get<uint32_t>();
                item.w.id = subJ["id"].get<uint32_t>();
                if (item.w.world > 255 || item.w.type > 255 || item.w.id > 255) {
                    printf("world/type/id is over limit 255\n");
                    return false;
                }
            }
            if (type & 2) {
                item.protocol = subJ["protocol"].get<std::string>();
            }
            if (type & 4) {
                item.ip = subJ["ip"].get<std::string>();
                item.port = subJ["port"].get<uint32_t>();
            }
            
            info.itemVec.push_back(item);
            info.itemMap[item.w] = item;
        }

        return true;
    };

    try {
        // parse server.json
        std::ifstream ifs(mConfFile);
        auto serveJson = nlohmann::json::parse(ifs, nullptr);

        // parse zm_listen
        if (!parseLinkInfo(serveJson["zm_listen"], mZmListen, 1|2|4)) {
            return false;
        }
        if (!parseLinkInfo(serveJson["zm_connect"], mZmConnect, 1|2|4)) {
            return false;
        }
        if (!parseLinkInfo(serveJson["net_listen"], mNetListen, 1|2|4)) {
            return false;
        }
        if (!parseLinkInfo(serveJson["net_connect"], mNetConnect, 1|2|4)) {
            return false;
        }
        if (!parseLinkInfo(serveJson["http_listen"], mHttpListen, 1|4)) {
            return false;
        }
        if (!parseLinkInfo(serveJson["routes"], mRoutes, 1)) {
            return false;
        }
        
    }
    catch(nlohmann::detail::exception& e) {
        printf("failed to parse %s|%s\n", mConfFile.c_str(), e.what());
        return false;
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