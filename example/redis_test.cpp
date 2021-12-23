#ifdef USE_ASYNC_REDIS

#include "./common.h"

extern bool print_log;

// redis测试
void redis_test(bool use_co, int& count, TimeElapsed& te, const char* url) {
    // 使用协程
    if (use_co) {
        // 启动一个协程任务
        CoroutineTask::doTask([&count, &te, url](void*) {
            static int fail_count = 0;
            static int timeout_count = 0;
            static int success_count = 0;

            // 执行redis访问并等待协程返回
            std::string value;
            int ret = co_async::redis::execute(url, async::redis::GetRedisCmd("mytest"), value);
            if (ret == co_bridge::E_CO_RETURN_TIMEOUT) {
                timeout_count++;
            }
            else if (ret == co_bridge::E_CO_RETURN_ERROR) {
                fail_count++;
            }
            else if (ret == co_bridge::E_CO_RETURN_OK) {
                success_count++;
                if (print_log) {
                    std::cout << "redis value:" << value << std::endl;
                }
            }

            count--;
            if (count == 0) {
                std::cout << "redis elapsed: " << te.end() 
                          << ", success_count:" << success_count 
                          << ", timeout_count:" << timeout_count 
                          << ", fail_count:" << fail_count << std::endl;
            }
        }, 0);
    }
    // 不使用协程
    else {
        // 发起一个redis异步访问并期待回调被调用
        async::redis::execute(url, async::redis::GetRedisCmd("mytest"), [&count, &te](async::redis::RedisReplyParserPtr ptr) {
            count--;
            std::string value;
            ptr->GetString(value);
            if (print_log) {
                std::cout << "redis value:" << value << std::endl;
            }

            if (count == 0) {
                std::cout << "redis elapsed: " << te.end() << std::endl;
            }
        });
    }
}

#endif