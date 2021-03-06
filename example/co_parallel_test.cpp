#ifdef USE_ASYNC_CURL

#include "./common.h"

void co_parallel_test() {
    // 启动一个协程任务
    CoroutineTask::doTask([](void*) {
        // 异步io并行访问，等待访问结束时协程被唤醒
        auto ret = co_async::parallel(
        {
            [](co_async::next_cb next) {
                std::cout << "begin1" << std::endl;
                async::curl::get("http://baidu.com", [next](int, int, std::string& body) {
                    std::cout << "parallel body3:" << body.length() <<std::endl;
                    next(1);
                });
            },
            [](co_async::next_cb next) {
                std::cout << "begin2" << std::endl;
                async::curl::get("http://192.168.0.22:5003", [next](int, int, std::string& body) {
                    std::cout << "parallel body4:" << body.length() <<std::endl;
                    next(2);
                });
            }
        });
        std::cout << "parallel over:" << ret.first << "|" << ret.second << std::endl;
    }, 0);

}

#endif