/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_RABBITMQ

#include "common/async/rabbitmq/rabbitmq_cmd.h"

namespace async {
namespace rabbitmq {

DeclareExchangeCmd::DeclareExchangeCmd() {
    this->cmdType = declare_exchange_cmd;
}

DeclareExchangeCmd::DeclareExchangeCmd(const std::string& name, 
    const std::string& type,
    bool passive,
    bool durable,
    bool autoDelete
)
{
    this->cmdType = declare_exchange_cmd;
    this->name = name;
    this->type = type;
    this->passive = passive;
    this->durable = durable;
    this->autoDelete = autoDelete;
}

DeclareQueueCmd::DeclareQueueCmd() {
    this->cmdType = declare_queue_cmd;
}

DeclareQueueCmd::DeclareQueueCmd(const std::string& name,
    bool passive,
    bool durable,  
    bool exclusive,
    bool autoDelete
)
{
    this->cmdType = declare_queue_cmd;
    this->name = name;
    this->passive = passive;
    this->durable = durable;
    this->exclusive = exclusive;
    this->autoDelete = autoDelete;
}

BindCmd::BindCmd() {
    this->cmdType = bind_cmd;
}

BindCmd::BindCmd(const std::string& exchange, const std::string& queue, const std::string& routingKey) {
    this->cmdType = bind_cmd;
    this->exchange = exchange;
    this->queue = queue;
    this->routingKey = routingKey;
}

UnbindCmd::UnbindCmd() {
    this->cmdType = unbind_cmd;
}

UnbindCmd::UnbindCmd(const std::string& exchange, const std::string& queue, const std::string& routingKey) {
    this->cmdType = unbind_cmd;
    this->exchange = exchange;
    this->queue = queue;
    this->routingKey = routingKey;
}

PublishCmd::PublishCmd() {
    this->cmdType = publish_cmd;
}

PublishCmd::PublishCmd(const std::string& msg, 
    const std::string& exchange, 
    const std::string& routingKey,
    bool mandatory,
    bool immediate)
{
    this->cmdType = publish_cmd;
    this->msg = msg;
    this->exchange = exchange;
    this->routingKey = routingKey;
    this->mandatory = mandatory;
    this->immediate = immediate;
}

WatchCmd::WatchCmd() {
    this->cmdType = watch_cmd;
}

WatchCmd::WatchCmd(const std::string& queue,        
    const std::string& consumer_tag,
    bool no_local,    
    bool no_ack,    
    bool exclusive
)
{
    this->cmdType = watch_cmd;
    this->queue = queue;
    this->consumer_tag = consumer_tag;
    this->no_local = no_local;
    this->no_ack = no_ack;
    this->exclusive = exclusive;
}

UnWatchCmd::UnWatchCmd() {
    cmdType = unwatch_cmd;
}

DeleteQueueCmd::DeleteQueueCmd() {
    cmdType = delete_queue_cmd;
}

DeleteExchangeCmd::DeleteExchangeCmd() {
    cmdType = delete_exchange_cmd;
}

GetCmd::GetCmd() {
    cmdType = get_cmd;
}

AckCmd::AckCmd() {
    this->cmdType = ack_cmd;
}

AckCmd::AckCmd(const std::string& queue, const std::string& consumer_tag, uint64_t delivery_tag) {
    this->cmdType = ack_cmd;
    this->queue = queue;
    this->consumer_tag = consumer_tag;
    this->delivery_tag = delivery_tag;
}

}
}

#endif