#ifdef USE_ASYNC_RABBITMQ

#include "./common.h"

void rabbit_test2(bool use_co) {
    auto watchCmd = std::make_shared<async::rabbitmq::WatchCmd>();
    watchCmd->queue = "queue1";
    watchCmd->consumer_tag = "consumer_tag";
    watchCmd->no_ack = true;
    async::rabbitmq::watch("localhost|5672|/|admin|admin", watchCmd, [](void* reply, void* envelope, uint64_t delivery_tag, char* body, size_t len) {
        std::string msg(body, len);
        log("msg1: %s", msg.c_str());
    });

    static int watchCmdCnt = 0;
    auto watchCmd2 = std::make_shared<async::rabbitmq::WatchCmd>();
    watchCmd2->queue = "queue2";
    watchCmd2->no_ack = true;
    async::rabbitmq::watch("localhost|5672|/|admin|admin", watchCmd2, [&watchCmdCnt](void* reply, void* envelope, uint64_t delivery_tag, char* body, size_t len) {
        std::string msg(body, len);
        log("msg2: %s", msg.c_str());
        watchCmdCnt++;

        if (watchCmdCnt == 2) {
            auto unwatchCmd = std::make_shared<async::rabbitmq::UnWatchCmd>();
            unwatchCmd->queue = "queue2";
            async::rabbitmq::unwatch("localhost|5672|/|admin|admin", unwatchCmd);
        }
    });

    auto watchCmd3 = std::make_shared<async::rabbitmq::WatchCmd>();
    watchCmd3->queue = "queue1";
    watchCmd3->consumer_tag = "consumer_tag2";
    watchCmd3->no_ack = true;
    async::rabbitmq::watch("localhost|5672|/|admin|admin", watchCmd3, [](void* reply, void* envelope, uint64_t delivery_tag, char* body, size_t len) {
        std::string msg(body, len);
        log("msg3: %s", msg.c_str());
    });


    return;

    auto exchangeCmd = std::make_shared<async::rabbitmq::DeclareExchangeCmd>();
    exchangeCmd->name = "direct.exchange1";
    exchangeCmd->type = "direct";
    //exchangeCmd->passive = true;
    async::rabbitmq::execute("localhost|5672|/|admin|admin", exchangeCmd, [](void* reply, bool ok) {
        if (ok) {
            log("declare exchange ok");
        } else {
            log("declare exchange fail");
        }
    }); 

    //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    //return;

    auto queueCmd = std::make_shared<async::rabbitmq::DeclareQueueCmd>();
    queueCmd->name = "queue1";
    //queueCmd->passive = true;
    async::rabbitmq::execute("localhost|5672|/|admin|admin", queueCmd, [](void* reply, bool ok) {
        if (ok) {
            log("declare queue ok");
        } else {
            log("declare queue fail");
        }
    });

    //return;
    //std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    auto bindCmd = std::make_shared<async::rabbitmq::BindCmd>();
        bindCmd->exchange = "direct.exchange1";
        bindCmd->queue = "queue1";
        bindCmd->routingKey = "test";

        async::rabbitmq::execute("localhost|5672|/|admin|admin", bindCmd, [](void* reply, bool ok) {
            if (ok) {
                log("bind queue ok");
            } else {
                log("bind queue fail");
            }
    });

    auto publishCmd = std::make_shared<async::rabbitmq::PublishCmd>();
    publishCmd->msg = "this is a message from test";
    publishCmd->exchange = "direct.exchange1";
    publishCmd->routingKey = "test";

    async::rabbitmq::execute("localhost|5672|/|admin|admin", publishCmd, [](void*, bool ok) {
        if (ok) {
                log("push msg ok");
            } else {
                log("push msg fail");
            }
    });
}

void rabbit_test(bool use_co) {
    std::string uri = "localhost|5672|/|admin|admin";
    // 使用协程
    CoroutineTask::doTask([uri](void*) {
        return;
        auto exchangeCmd = std::make_shared<async::rabbitmq::DeclareExchangeCmd>();
        exchangeCmd->name = "direct.exchange1";
        exchangeCmd->type = "direct";

        auto ret = co_async::rabbitmq::execute(uri, exchangeCmd);
        if (ret.first == co_async::E_OK) {
            log("declare exchange: %d", ret.second);
        } else {
            log("declare exchange timeout or error: %d", ret.first);
        } 

        auto queueCmd = std::make_shared<async::rabbitmq::DeclareQueueCmd>();
        queueCmd->name = "queue1";
        ret = co_async::rabbitmq::execute(uri, queueCmd);
        if (ret.first == co_async::E_OK) {
            log("declare queue: %d", ret.second);
        } else {
            log("declare queue timeout or error: %d", ret.first);
        } 

        auto bindCmd = std::make_shared<async::rabbitmq::BindCmd>();
        bindCmd->exchange = "direct.exchange1";
        bindCmd->queue = "queue1";
        bindCmd->routingKey = "test";
        ret = co_async::rabbitmq::execute(uri, bindCmd);
        if (ret.first == co_async::E_OK) {
            log("bind queue: %d", ret.second);
        } else {
            log("bind queue timeout or error: %d", ret.first);
        } 

        for (int i = 0; i < 10000; i++) {
            auto publishCmd = std::make_shared<async::rabbitmq::PublishCmd>();
            publishCmd->msg = "this is a message " + std::to_string(i);
            publishCmd->exchange = "direct.exchange1";
            publishCmd->routingKey = "test";

            co_async::rabbitmq::execute(uri, publishCmd);
        }

    }, 0);

    // for (int i = 0; i < 10000; i++) {
    //     auto publishCmd = std::make_shared<async::rabbitmq::PublishCmd>();
    //     publishCmd->msg = "this is a message " + std::to_string(i);
    //     publishCmd->exchange = "direct.exchange1";
    //     publishCmd->routingKey = "test";

    //     async::rabbitmq::execute(uri, publishCmd, [](void* reply, bool ok) {
    //         static int count = 0;
    //         count++;
    //         if (count == 10000) {
    //             log("%d %d", count, ok);
    //         }
    //     });
    // }

    // auto getCmd = std::make_shared<async::rabbitmq::GetCmd>();
    // getCmd->queue = "queue1";
    // getCmd->no_ack = false;
    // async::rabbitmq::execute(uri, getCmd, [](void*, void* message, bool ok, char* body, size_t len) {
    //     if (!ok) {
    //         log("error");
    //         return;
    //     }
    //     if (!body) {
    //         log("get empty: %d", len);
    //     } else {
    //         std::string msg(body, len);
    //         log("get: %s", msg.c_str());
    //     }
    // });

    auto watchCmd = std::make_shared<async::rabbitmq::WatchCmd>();
    watchCmd->queue = "queue1";
    watchCmd->consumer_tag = "consumer_tag";
    watchCmd->no_ack = false;
    async::rabbitmq::watch(uri, watchCmd, [uri](void* reply, void* envelope, uint64_t delivery_tag, char* body, size_t len) {
        static int count = 0;
        count++;
        std::string msg(body, len);
        log("msg: %s", msg.c_str());
        if (count < 3) {
            auto ackCmd = std::make_shared<async::rabbitmq::AckCmd>();
            ackCmd->delivery_tag = delivery_tag;
            ackCmd->queue = "queue1";
            ackCmd->consumer_tag = "consumer_tag";
            async::rabbitmq::watchAck(uri, ackCmd, [delivery_tag](void* reply, bool ok) {
                if (ok) {
                    log("ack ok: %ld", delivery_tag);
                } else {
                    log("ack fail: %ld", delivery_tag);
                }
            });
        }
    });

}

#endif