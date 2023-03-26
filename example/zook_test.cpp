#ifdef USE_ASYNC_ZOOKEEPER

#include "./common.h"

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

void zookeeper_test(bool co) {
    std::string uri = "192.168.31.86:2181";
    //batchGet(uri);
    create(uri);
    set(uri);

    co_async::setTimeout([uri]() {
        create(uri);
    }, 5*1000);
}

#endif