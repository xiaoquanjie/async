#pragma once

#include <iostream>
#include <sys/time.h>
#include <unistd.h>
#include "common/coroutine/coroutine.hpp"
#include "common/co_bridge/co_bridge.h"
#include "common/ipc/zero_mq_handler.h"
#include "common/async/async.h"
#include "common/net/tcp_client.h"
#include "common/net/tcp_listener.h"
#include "common/net/udp_listener.h"
#include "common/net/udp_client.h"
#include "common/threads/thread_pool.h"
#include "common/co_async//ipc/co_ipc.h"

// 计时器
struct TimeElapsed {
    void begin() {
        gettimeofday(&tv, NULL);
    }

    uint64_t end() {
        struct timeval tv2;
        gettimeofday(&tv2, NULL);
	    return (tv2.tv_sec * 1000 + tv2.tv_usec / 1000) - (tv.tv_sec * 1000 + tv.tv_usec / 1000);
    }

    struct timeval tv;
};