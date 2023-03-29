#ifdef USE_ASYNC_ZOOKEEPER

#include "./common.h"
#include <algorithm>

namespace co_zook = co_async::zookeeper;
namespace zook = async::zookeeper;

void batchGet(std::string uri) {
    for (int i = 0; i < 100; i++) {
        std::string node = "/testnode1" + std::to_string(i % 2);
        async::zookeeper::execute(uri, std::make_shared<async::zookeeper::GetCmd>(node), [](async::zookeeper::ZookParserPtr parser) {
            if (parser->zTimeout()) {
                log("timeout");
            }
            else if (parser->zOk()) {
                log("data: %s", parser->getValue().c_str());
            }
            else {
                log("rc: %d", parser->getRc());
            }
        });
    }

    co_async::setTimeout([uri]() {
        batchGet(uri);
    }, 20 * 1000);
}

void set(std::string uri) {
    async::zookeeper::execute(uri, std::make_shared<async::zookeeper::SetCmd>(
        "/testnode100",
        "this is testnode100"
    ), [](async::zookeeper::ZookParserPtr parser) {
        if (parser->zTimeout()) {
            log("timeout");
        } else if (parser->zOk()) {
            log("ok");
        } else {
            log("fail");
        }
    });
}

void create(std::string uri) {
    auto cmd = std::make_shared<async::zookeeper::CreateCmd>(
        "/jack",
        "oh, shit",
        true,
        true,
        0
    );
    async::zookeeper::execute(uri, cmd, [](async::zookeeper::ZookParserPtr parser) {
        if (parser->zTimeout()) {
            log("timeout");
        } else if (parser->zOk()) {
            log("ok: %s", parser->getValue().c_str());
        } else {
            log("fail");
        }
    });
}

void zook_non_co_test() {

}

void zook_reg_servicenames(std::string uri, std::string serviceNames) {
    // 服务注册
    auto res = co_zook::execute(uri, std::make_shared<zook::ExistsCmd>(serviceNames));
    if (res.first != co_async::E_OK) {
        log("timeout to exist");
        return;
    }

    auto parser = res.second;
    if (!parser->zOk()) {
        if (!parser->zNoNode()) {
            log("failed to exist");
            return;
        }

        // 创建一个
        res = co_zook::execute(uri, std::make_shared<zook::CreateCmd>(serviceNames, ""));
        if (res.first != co_async::E_OK) {
            log("timeout to create");
            return;
        }

        parser = res.second;
        if (!parser->zOk()) {
            log("failed to create");
            return;
        }
    }

    std::string serviceInfo = "/127.0.0.1:5462";
    bool ok = false;
    int ret = co_zook::execute(uri, std::make_shared<zook::CreateCmd>(serviceNames + serviceInfo, "", true, false, 0), ok);
    if (ret != co_async::E_OK) {
        log("timeout to create service node");
        return;
    }

    if (!ok) {
        log("failed to create service node");
        return;
    }

    log("createe service node: %s", (serviceNames + serviceInfo).c_str());
}

void zook_poll_servicenames(std::string uri, std::string serviceNames) {
    std::vector<std::string>* services_info = new std::vector<std::string>();
    zook::watchChild(uri, serviceNames, [services_info](zook::ZookParserPtr parser) {
        if (parser->zOk()) {
            services_info->clear();
            for (auto v : parser->getChilds()) {
                services_info->push_back(*v);
            }
        }
        else {
            services_info->clear();
            log("error");
        }
    });

    for (;;) {
        co_async::wait(1000);
        log("services:");
        for (auto s : *services_info) {
            log("service: %s", s.c_str());
        }
    }
}

void zook_co_lock_test(int seq, std::string uri) {
    // 创建临时节点
    auto res = co_zook::execute(uri, std::make_shared<zook::CreateCmd>(
        "/lock/program",
        std::to_string(seq),
        true,
        true,
        0
    ));

    if (!res.second || !res.second->zOk()) {
        log("failed to create node, quit: %d", seq);
        return;
    }

    std::string selfNode = res.second->getValue();
    log("create node: %s, seq: %d", selfNode.c_str(), seq);

    // 获取所有的子节点
    res = co_zook::execute(uri, std::make_shared<zook::ListCmd>(
        "/lock"
    ));

    if (!res.second || !res.second->zOk()) {
        log("failed to get child node, quit: %d", seq);
        return;
    }
    
    // 获取比本节点次小的节点名
    std::string* lastNode = new std::string();
    std::vector<std::string> all;
    for (auto v : res.second->getChilds()) {
        all.push_back(*v);
    }

    std::sort(all.begin(), all.end());
    for (auto& v: all) {
        if ((std::string("/lock/") + v) == selfNode) {
            break;
        }
        *lastNode = std::string("/lock/") + v;
    }

    if (!lastNode->empty()) {
        // 监听它
        log("watch node: %s, seq: %d", lastNode->c_str(), seq);
        zook::watchOnce(uri, *lastNode, [lastNode, seq](zook::ZookParserPtr parser) {
            if (parser->zNoNode()) {
                log("seq: %d lastnode free: %s", seq, parser->error());
                lastNode->clear();
            }
            else {
                log("seq: %d, %s, %s", seq, lastNode->c_str(), parser->error());
            }
        });
    }
    else {
        log("i am the min node, seq: %d", seq);
    }
    
    for(;;) {
        if (!lastNode->empty()) {
            co_async::wait(10);
            continue;
        }
        break;
    }

    log("got the lock, seq: %d, node: %s", seq, selfNode.c_str());

    // 删除节点
    res = co_zook::execute(uri, std::make_shared<zook::DeleteCmd>(selfNode));
    if (res.second) {
        log("delete the lock, seq: %d, node: %s", seq, selfNode.c_str());
    }
    delete lastNode;
}

void zook_co_watch_test(std::string uri) {
    zook::watchOnce(uri, "/lock/1", [](zook::ZookParserPtr parser) {
        log("/lock/1 %s", parser->error());
    });

    zook::watchOnce(uri, "/lock/2", [](zook::ZookParserPtr parser) {
        log("/lock/2 %s", parser->error());
    });
}

void zook_co_test(void* d) 
{
    int& seq = *((int*)d);
    std::string uri = "127.0.0.1:2181";
    std::string serviceNames = "/serviceNames";

    //zook_co_watch_test(uri);
    zook_co_lock_test(seq, uri);
    //zook_reg_servicenames(uri, serviceNames);
    //zook_poll_servicenames(uri, serviceNames);
    
}

void zookeeper_test(bool co, int seq) {
    CoroutineTask::doTask(zook_co_test, new int(seq));
    //zook_non_co_test();
}

#endif