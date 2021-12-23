#include "./common.h"

extern bool print_log;

void cpu_test(bool use_co, int& count, TimeElapsed& te) {
    // 使用协程
    if (use_co) {
        // 启动一个协程任务
        CoroutineTask::doTask([&count, &te](void*) {
            static int fail_count = 0;
            static int timeout_count = 0;
            static int success_count = 0;

            // 执行cpu访问并等待协程返回
            // cpu计算
            int64_t result = 0;
            int ret = co_async::cpu::execute([](void*) {
                int count = 100000000;
                int64_t t = 0;
                while (count--) {
                    t += count;
                }
                return t;
            },
            0,
            [count, &result](int64_t res, void*) {
                result += res;
            });

            if (ret == co_bridge::E_CO_RETURN_TIMEOUT) {
                timeout_count++;
            }
            else if (ret == co_bridge::E_CO_RETURN_ERROR) {
                fail_count++;
            }
            else if (ret == co_bridge::E_CO_RETURN_OK) {
                success_count++;
            }
                
            count--;
            if (count == 0) {
                std::cout << "cpu elapsed: " << te.end() 
                          << ", success_count:" << success_count 
                          << ", timeout_count:" << timeout_count 
                          << ", fail_count:" << fail_count << std::endl;
            }
        }, 0);
    }
    // 不使用协程
    else {
        // 发起一个cpu异步运算并期待回调被调用
        int64_t result = 0;
        async::cpu::execute([](void*) {
            int count = 100000000;
            int64_t t = 0;
            while (count--) {
            t += count;
            }
            return t;
        },
        0,
        [&count, &te, &result](int64_t res, void*) {
            count--;
            result += res;
            if (count == 0) {
                std::cout << "cpu elapsed: " << te.end() << "|" << result << std::endl;
            }
        });
    }
}