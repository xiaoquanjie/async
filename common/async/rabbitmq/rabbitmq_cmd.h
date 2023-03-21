/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include <string>
#include <unordered_map>

namespace async {
namespace rabbitmq {

enum cmd_type {
    declare_exchange_cmd = 1,   // 声明交换机
    declare_queue_cmd = 2,      // 声明队列 
    bind_cmd = 3,               // 绑定交换机和队列
    unbind_cmd = 4,             // 解绑交换机和队列
    publish_cmd = 5,            // 发布
    watch_cmd = 6,              // 监听
    delete_queue_cmd = 7,       // 删除队列
    delete_exchange_cmd = 8,    // 删除交换机
};

struct BaseRabbitCmd {
    uint32_t cmdType = 0;
    virtual ~BaseRabbitCmd() {}
};

// 声明交换机
struct DeclareExchangeCmd : public BaseRabbitCmd {
    DeclareExchangeCmd();

    std::string name;               // 名字
    std::string type;               // 类型
    bool        passive = false;    // true的话，则不会创建交换机，相当于open
    bool        durable = false;    // true的话，则为持久化
    bool        autoDelete = false; // true的话，如果没有队列和交换机绑定，则交换机删除
};

// 声明消息队列
struct DeclareQueueCmd : public BaseRabbitCmd {
    DeclareQueueCmd();

    std::string name;               // 名字
    bool        passive = false;    // true的话，则不会创建队列，相当于open
    bool        durable = false;    // true的话，则为持久化
    bool        exclusive = false;  // true的话，具有排他性，连接（第一个创建它的连接）断开时，队列被删除。可做临时队列
    bool        autoDelete = false; // true的话，如果没有队列和交换机绑定，则队列删除
};

// 绑定
struct BindCmd : public BaseRabbitCmd {
    BindCmd();

    std::string exchange;   // 交换机 
    std::string queue;      // 队列
    std::string routingKey; // 路由
};

// 解绑
struct UnbindCmd : public BaseRabbitCmd {
    UnbindCmd();

    std::string exchange;   // 交换机 
    std::string queue;      // 队列
    std::string routingKey; // 路由
};

// 发布
struct PublishCmd : public BaseRabbitCmd {
    PublishCmd();

    std::string msg;        // 消息
    std::string exchange;   // 交换机 
    std::string routingKey; // 路由
    bool mandatory = false; // true消息必须路由到存在的队列，否则返回失败
    bool immediate = false; // true如果消息没有订阅，返回失败

    /**
        属性, 具体参考amqp_basic_properties_t    
        常用的有：
            content-type属性用于描述消息体的数据格式
            content-encoding属性
            message-id
            timestamp属性
            expiration属性
            delivery-mode属性：1表示非持久化消息，2表示持久化消息
            priority属性优先级，值为0~9
    */ 
    std::unordered_map<std::string, std::string> properties;   
};

// 监听
struct WatchCmd : public BaseRabbitCmd {
    WatchCmd();

    std::string queue;          // 队列
    std::string consumer_tag;   // 用于区分不同的消费者
    bool no_local = false;      // 1不接收 0接收
    bool no_ack = true;         // true: 不需要ack就会删除
    bool exclusive = false;     // 1当前连接不在时，队列自动删除 0当前连接不在时，队列不自动删除
};

// 删除
struct DeleteQueueCmd : public BaseRabbitCmd {
    DeleteQueueCmd();

    std::string name;               // 名字
    bool if_unused = false;         // 队列不用后删掉
    bool if_empty = false;          // 队列空后删掉
};

struct DeleteExchangeCmd : public BaseRabbitCmd {
    DeleteExchangeCmd();

    std::string name;               // 名字
    bool if_unused = false;         // 交换机不用后删掉
};

}
}