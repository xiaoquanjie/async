#ifdef USE_ASYNC_CURL

#include "./common.h"

extern bool print_log;

// http测试
void curl_test(bool use_co, int& count, TimeElapsed& te, const char* url) {
    // 使用协程
    if (use_co) {
        // 启动一个协程任务
        CoroutineTask::doTask([&count, &te, url](void*) {
            static int fail_count = 0;
            static int timeout_count = 0;
            static int success_count = 0;

            // 执行http访问并等待协程返回
            std::string body;
            int ret = co_async::curl::get(url, [&count, &te, &body](int, int, std::string& b) {
                body.swap(b);
            });
            if (ret == co_bridge::E_CO_RETURN_TIMEOUT) {
                timeout_count++;
            }
            else if (ret == co_bridge::E_CO_RETURN_ERROR) {
                fail_count++;
            }
            else if (ret == co_bridge::E_CO_RETURN_OK) {
                success_count++;
                if (print_log) {
                    std::cout << "curl body.len: " << body.length() << std::endl;
                }
            }

            count--;
            if (count == 0) {
                std::cout << "curl elapsed: " << te.end() 
                          << ", success_count:" << success_count 
                          << ", timeout_count:" << timeout_count 
                          << ", fail_count:" << fail_count << std::endl;
            }
        }, 0);
    }
    // 不使用协程
    else {
        // 发起一个http异步访问并期待回调被调用
        async::curl::get(url, [&count, &te](int, int, const std::string& body) {
            count--;
            if (print_log) {
                std::cout << "curl body.len: " << body.length() << std::endl;
            }
            if (count == 0) {
                std::cout << "curl elapsed: " << te.end() << std::endl;
            }
        });
    }
}

#endif