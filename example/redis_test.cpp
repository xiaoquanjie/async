#ifdef USE_ASYNC_REDIS

#include "./common.h"

extern bool print_log;

// redis测试
void redis_test(bool use_co) {
    // 使用协程
    if (use_co) {
        // 启动一个协程任务
        CoroutineTask::doTask([](void*) {
            // 执行redis访问并等待协程返回
            {
                // ok
                std::string value;
                auto ret = co_async::redis::execute("192.168.0.22|6379||0|", async::redis::GetRedisCmd("mytest"), value);
                std::cout << ret.first << "|" << ret.second << "|" << value << std::endl; 
            }
            {
                // fail
                std::string value;
                auto ret = co_async::redis::execute("192.168.0.22|6378||0|", async::redis::GetRedisCmd("mytest"), value, co_async::TimeOut(1000));
                std::cout << ret.first << "|" << ret.second << "|" << value << std::endl; 
            }

        }, 0);
    }
    // 不使用协程
    else {
        // 发起一个redis异步访问并期待回调被调用
        async::redis::execute("192.168.0.22|6379||0|", async::redis::GetRedisCmd("mytest"), [](async::redis::RedisReplyParserPtr ptr) {
            std::string value;
            ptr->GetString(value);
            std::cout << "redis value:" << value << std::endl;
        });
    }
}

#endif