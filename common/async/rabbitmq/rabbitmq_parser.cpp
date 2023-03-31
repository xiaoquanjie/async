/*----------------------------------------------------------------
// Copyright 2023
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_RABBITMQ

#include "common/async/rabbitmq/rabbitmq_parser.h"
#include <rabbitmq-c/amqp.h>

namespace async {
namespace rabbitmq {

#define ENVELOPE ((amqp_envelope_t*)this->envelope)
#define MESSAGE ((amqp_message_t*)this->message)
#define REPLY ((amqp_rpc_reply_t*)this->reply)

RabbitParser::RabbitParser() {
    this->reply = malloc(sizeof(amqp_rpc_reply_t));
    REPLY->reply_type = AMQP_RESPONSE_NORMAL;
    this->envelope = 0;//malloc(sizeof(amqp_envelope_t));
    this->message = 0;//malloc(sizeof(amqp_message_t));
}
   
RabbitParser::~RabbitParser() {
    if (this->envelope) {
        amqp_destroy_envelope(ENVELOPE);
        free(this->envelope);
    }
    if (this->message) {
        amqp_destroy_message(MESSAGE);
        free(this->message);
    }
    
    free(this->reply);
}

bool RabbitParser::isOk() {
    if (this->timeout) {
        return false;
    }

    return REPLY->reply_type == AMQP_RESPONSE_NORMAL && this->x == AMQP_STATUS_OK;
}

bool RabbitParser::isTimeout() {
    return this->timeout;
}

uint64_t RabbitParser::getDeliveryTag() {
    if (ENVELOPE) {
        return ENVELOPE->delivery_tag;
    }
    return 0;
}

char* RabbitParser::getBody() {
    if (ENVELOPE) {
        return (char*)ENVELOPE->message.body.bytes;
    }
    if (MESSAGE) {
        return (char*)MESSAGE->body.bytes;
    }
    return nullptr;
}

size_t RabbitParser::getBodyLen() {
    if (ENVELOPE) {
        return ENVELOPE->message.body.len;
    }
    if (MESSAGE) {
        return MESSAGE->body.len;
    }
    return 0;
}

}
}

#endif