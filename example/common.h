#pragma once

#include <iostream>
#include <sys/time.h>
#include <unistd.h>
#include "async/coroutine/coroutine.hpp"
#include "async/co_async/async.h"
#include "async/co_async/cpu/co_cpu.h"
#include "async/co_async/curl/co_curl.h"
#include "async/co_async/mongo/co_mongo.h"
#include "async/co_async/redis/co_redis.h"
#include "async/co_async/mysql/co_mysql.h"
#include "async/ipc/zero_mq_handler.h"
#include "async/async/async.h"
#include "async/net/tcp_client.h"
#include "async/net/tcp_listener.h"
#include "async/net/udp_listener.h"
#include "async/net/udp_client.h"
#include "async/threads/thread_pool.h"
#include "async/co_async//ipc/co_ipc.h"
#include "async/co_async/rabbitmq/co_rabbitmq.h"
#include "async/async/zookeeper/async_zookeeper.h"
#include "async/co_async/zookeeper/co_zookeeper.h"

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