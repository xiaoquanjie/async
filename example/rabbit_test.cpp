#ifdef USE_ASYNC_RABBITMQ

#include "./common.h"

void rabbit_test(bool use_co) {
    auto watchCmd = std::make_shared<async::rabbitmq::WatchCmd>();
    watchCmd->queue = "queue1";
    watchCmd->consumer_tag = "consumer_tag";
    async::rabbitmq::watch("localhost|5672|/|admin|admin", watchCmd, [](void* reply, void* envelope, char* body, size_t len) {
        std::string msg(body, len);
        log("msg: %s", msg.c_str());
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

#endif