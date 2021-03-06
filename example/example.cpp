#include <stdio.h>
#include <memory>
#include "common/co_async/async.h"
#include "common/co_async/promise.h"
#include "common/coroutine/coroutine.hpp"
#include <unistd.h>

////////////////////
bool print_log = false;

void co_parallel_test();
void net_test();
void cpu_test(bool use_co);
void curl_test(bool use_co);
void mongo_test(bool use_co);
void redis_test(bool use_co);
void co_mysql_test();
void ipc_test();

void promise_test() {
    CoroutineTask::doTask([](void*){
        printf("promise begin\n");
        auto ret = co_async::promise([](co_async::Resolve resolve, co_async::Reject reject) {
            co_async::setTimeout([resolve]() {
                printf("异步执行结束\n");
                resolve(nullptr);
            }, 4*1000);
        }, 3 * 1000);

        printf("promise over:%d\n", ret.first);
    }, 0);
}

int main() {
    //promise_test();
    //cpu_test(true);
    //co_parallel_test();
    //curl_test(true);
    //mongo_test(true);
    //redis_test(true);
    //co_mysql_test();
    //ipc_test();

    while (true) {
        co_async::loop(time(0));
        usleep(100);
        //printf("loop\n");
    }
    return 0;
}

