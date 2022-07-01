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

void promise_test() {
    CoroutineTask::doTask([](void*){
        auto ret = co_async::promise([](co_async::Resolve resolve, co_async::Reject reject) {

        });
        printf(".....\n");
        printf("promise over:%d\n", ret.first);
    }, 0);
}

int main() {
    //promise_test();
    //cpu_test(true);
    //co_parallel_test();
    curl_test(true);

    while (true) {
        co_async::loop();
        usleep(100);
        //printf("loop\n");
    }
    return 0;
}

