/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_IPC

#include "serve/serve/router.h"
#include "common/ipc/ipc.h"
#include <set>
#include <map>

namespace router {

// worldid-->uniqueId
std::map<uint32_t, uint64_t> gUniqueMap;

// worldid-->(type--->world)
std::map<uint32_t, std::map<uint32_t, std::vector<World>>> gRoutesMap;

std::function<bool(uint64_t, uint32_t, const std::string)> gMiddle = nullptr;

struct ZqRouter : public ipc::ZeromqRouterHandler {
    void onData(uint64_t uniqueId, uint32_t identify, const std::string &data) override
    {
        if (!(gMiddle && gMiddle(uniqueId, identify, data))) {
            const BackendMsg *msg = (const BackendMsg *)data.c_str();
            World w(msg->header.dstWorldId);

            if (w.type == 0) {
                return;
            }

            auto iter = gRoutesMap.find(w.world);
            if (iter == gRoutesMap.end()) {
                return;
            }

            auto iterType = iter->second.find(w.type);
            if (iterType == iter->second.end()) {
                return;
            }

            // 广播
            if (msg->header.broadcast == 1) {
                // 获取所有的type
                for (auto w : iterType->second) {
                    transfer(data, msg, w);
                }
            }
            else {
                if (w.id == 0) {
                    // 查找一个合适的
                    size_t idx = msg->header.targetId % iterType->second.size();
                    w.world = iterType->second[idx].world;
                    w.id = iterType->second[idx].id;
                }

                transfer(data, msg, w);
            }
        }
    }

    void transfer(const std::string& data, const BackendMsg* msg, World& w) {
        // 找一个连接
        auto iter = gUniqueMap.find(w.world);
        if (iter == gUniqueMap.end()) {
            return;
        }

        uint64_t uniqueId = iter->second;
        uint32_t dstWorldId = w.identify();

        BackendMsg* mutableMsg = const_cast<BackendMsg*>(msg);
        uint32_t oriWorldId = mutableMsg->header.dstWorldId;
        mutableMsg->header.dstWorldId = dstWorldId;

        // 发送
        this->sendData(uniqueId, dstWorldId, data);

        // 还原数据
        mutableMsg->header.dstWorldId = oriWorldId;
    }
};

ZqRouter gZqRouter;

bool init(const LinkInfo& link, const LinkInfo& routes) {
    std::set<World> routesSet;
    for (auto item : routes.itemVec) {
        routesSet.insert(item.w);
    }
    for (auto w : routesSet) {
        auto& tmp = gRoutesMap[w.world];
        tmp[w.type].push_back(w);
    }

    for (auto item : link.itemVec) {
        // 同一个世界的只监听一个
        if (gUniqueMap.find(item.w.world) != gUniqueMap.end()) {
            continue;
        }

        uint64_t id = gZqRouter.listen(item.addr());
        if (id == 0) {
            return false;
        }

        gUniqueMap[item.w.world] = id;
    }

    return true;
}

bool update(time_t now) {
    return gZqRouter.update(now);
}

void use(std::function<bool(uint64_t, uint32_t, const std::string&)> fn) {
    gMiddle = fn;
}

}

#endif