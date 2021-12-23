#ifdef USE_ASYNC_CURL

#include "./common.h"

void parallel_test() {
    // 异步io并行访问，并期待结束时回调被调用
    async::parallel([](int err) {
        std::cout << "parallel done:" << err << std::endl;
    },
    {
        [](async::next_cb next) {
            async::curl::get("http://baidu.com", [next](int, int, std::string& body) {
                std::cout << "parallel body1:" << body.length() <<std::endl;
                next(0);
            });
        },
        [](async::next_cb next) {
            async::curl::get("http://baidu.com", [next](int, int, std::string& body) {
                std::cout << "parallel body2:" << body.length() <<std::endl;
                next(2);
            });
        }
    });

    // 异步io串行访问，并期待结束时回调被调用
    async::series([](int err) {
        std::cout << "series done:" << err << std::endl;
    },
    {
        [](async::next_cb next) {
            async::curl::get("http://baidu.com", [next](int, int, std::string& body) {
                std::cout << "series body3:" << body.length() <<std::endl;
                next(0);
            });
        },
        [](async::next_cb next) {
            async::curl::get("http://baidu.com", [next](int, int, std::string& body) {
                std::cout << "series body4:" << body.length() <<std::endl;
                next(4);
            });
        }
    });

    while (true) {
        async::loop();
        sleep(1);
    }
}

#endif