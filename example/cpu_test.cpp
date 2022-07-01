#include "./common.h"

extern bool print_log;

void cpu_test(bool use_co) {
    // 使用协程
    if (use_co) {
        // 启动一个协程任务
        CoroutineTask::doTask([](void*) {
            auto ret = co_async::cpu::execute([](void*) {
                int count = 100000000;
                int64_t t = 0;
                while (count--) {
                    t += count;
                }
                return t;
            }, 0);

            std::cout << "co_cpu test, result=" << ret.second << std::endl;
        }, 0);
    }
    // 不使用协程
    else {
        // 发起一个cpu异步运算并期待回调被调用
        async::cpu::execute([](void*) {
            int count = 100000000;
            int64_t t = 0;
            while (count--) {
            t += count;
            }
            return t;
        },
        0,
        [](int64_t res, void*) {
            std::cout << "cpu test, result=" << res << std::endl;
        });
    }
}