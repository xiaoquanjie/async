/*----------------------------------------------------------------
// Copyright 2023
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include <memory>

namespace async {
namespace rabbitmq {

struct RabbitParser {
    RabbitParser();
   
    ~RabbitParser();

    bool isOk();

    bool isTimeout();

    uint64_t getDeliveryTag();

    char* getBody();

    size_t getBodyLen();

    int x = 0;
    void* reply;
    void* envelope;
    void* message;
    bool timeout = false;
};

typedef std::shared_ptr<RabbitParser> RabbitParserPtr;

}
}