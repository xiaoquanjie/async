/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_RABBITMQ

#include "common/async/rabbitmq/rabbitmq_cmd.h"

namespace async {
namespace rabbitmq {

DeclareExchangeCmd::DeclareExchangeCmd() {
    this->cmdType = declare_exchange_cmd;
}

DeclareQueueCmd::DeclareQueueCmd() {
    cmdType = declare_queue_cmd;
}

BindCmd::BindCmd() {
    cmdType = bind_cmd;
}

UnbindCmd::UnbindCmd() {
    cmdType = unbind_cmd;
}

PublishCmd::PublishCmd() {
    cmdType = publish_cmd;
}

WatchCmd::WatchCmd() {
    cmdType = watch_cmd;
}

DeleteQueueCmd::DeleteQueueCmd() {
    cmdType = delete_queue_cmd;
}

DeleteExchangeCmd::DeleteExchangeCmd() {
    cmdType = delete_exchange_cmd;
}

}
}

#endif