#ifdef USE_ASYNC_CURL

#include "./common.h"

void co_parallel_test() {
    // 启动一个协程任务
    CoroutineTask::doTask([](void*) {
        // 异步io并行访问，等待访问结束时协程被唤醒
        int ret = co_async::parallel(
        {
            [](co_async::next_cb next) {
                async::curl::get("http://baidu.com", [next](int, int, std::string& body) {
                    std::cout << "parallel body3:" << body.length() <<std::endl;
                    next();
                });
            },
            [](co_async::next_cb next) {
                async::curl::get("http://192.168.0.22:5003", [next](int, int, std::string& body) {
                    std::cout << "parallel body4:" << body.length() <<std::endl;
                    next();
                });
            }
        });
        std::cout << "parallel over" << ret << std::endl;
    }, 0);

    CoroutineTask::doTask([](void*) {
        int ret = co_async::series(
        {
            [](co_async::next_cb next) {
                async::curl::get("http://baidu.com", [next](int, int, std::string& body) {
                    std::cout << "series body1:" << body.length() <<std::endl;
                    next();
                });
            },
            [](co_async::next_cb next) {
                async::curl::get("http://192.168.0.22:5003", [next](int, int, std::string& body) {
                    std::cout << "series body2:" << body.length() <<std::endl;
                    next();
                });
            }
        });
        std::cout << "series over" << ret << std::endl;
    }, 0);


    while (true) {
        co_bridge::loop();
        sleep(1);
    }
}

#endif