#ifdef USE_ASYNC_RABBITMQ

#include "./common.h"

namespace co_rabbit = co_async::rabbitmq;
namespace rabbit = async::rabbitmq;

void rabbit_test2(bool use_co) {
    
}

void create_exchange(std::string uri) {
    rabbit::execute(uri, std::make_shared<rabbit::DeclareExchangeCmd>(
        "direct.exechange.1",
        "direct"
    ), [](rabbit::RabbitParserPtr parser) {
        if (parser->isOk()) {
            log("create exchange");
        }
        else {
            log("failed to create exchange");
        }
    });
}

void create_queue(std::string uri) {
    rabbit::execute(uri, std::make_shared<rabbit::DeclareQueueCmd>(
        "queue1",
        false,
        true
    ), [](rabbit::RabbitParserPtr parser) {
        if (parser->isOk()) {
            log("create queue");
        }
        else {
            log("failed to create queue");
        }
    });
}

void bind_queue(std::string uri) {
    rabbit::execute(uri, std::make_shared<rabbit::BindCmd>(
        "direct.exechange.1",
        "queue1",
        "route1"
    ), [](rabbit::RabbitParserPtr parser) {
        if (parser->isOk()) {
            log("bind queue");
        }
        else {
            log("failed to bind queue");
        }
    });
}

void publish(std::string uri) {
    for (int i = 0; i < 100; i++) {
        std::string msg = "this is msg ";
        msg += std::to_string(i);
        rabbit::execute(uri, std::make_shared<rabbit::PublishCmd>(
            msg,
            "direct.exechange.1",
            "route1"
        ), [i](rabbit::RabbitParserPtr parser) {
            if (parser->isOk()) {
                log("push msg: %d", i);
            }
            else {
                log("failed to push msg: %d", i);
            }
        });
    }
}

void consumer(std::string uri, int seq) {
    rabbit::watch(uri, std::make_shared<rabbit::WatchCmd>(
        "tqueue-mirror-1",
        "consumer_" + std::to_string(seq)
    ), [seq, uri](rabbit::RabbitParserPtr parser) {
        if (parser->isOk()) {
            std::string msg;
            if (parser->getBodyLen() != 0) {
                msg.append(parser->getBody(), parser->getBodyLen());
            }
            log("consumer: %d msg: %s", seq, msg.c_str());

            rabbit::watchAck(uri, std::make_shared<rabbit::AckCmd>(
                "tqueue-mirror-1",
                "consumer_" + std::to_string(seq),
                parser->getDeliveryTag()
            ), [](rabbit::RabbitParserPtr parser) {
                if (parser->isOk()) {
                    log("ack ok");
                }
                else {
                    log("ack fail");
                }
            });
        }
        else {
            log("consumer: %d error", seq);
        }
    });
}

void co_publish(std::string uri) {
    CoroutineTask::doTask([uri](void*) {
        int i = 1;
        for (;;) {
            std::string msg = "this is msg ";
            msg += std::to_string(i);
            auto res = co_rabbit::execute(uri, std::make_shared<rabbit::PublishCmd>(
                msg,
                "direct.exechange.1",
                "route2",
                true,
                false
            ));
            if (!(res.second && res.second->isOk())) {
                log("failed to publish");
            }
            else {
                log("publish ok");
            }
            co_async::wait(2000);
            i++;
            // break;
        }
    }, 0);
}

void rabbit_test(bool use_co, int seq) {
    std::string uri = "localhost|5676|/|admin|admin";
    // create_exchange(uri);
    // create_queue(uri);
    // bind_queue(uri);
    //publish(uri);
    //consumer(uri, seq);
    co_publish(uri);
}

#endif