#ifdef USE_ASYNC_MYSQL

#include "./common.h"

void mysql_test() {
    async::mysql::setKeepConnection(1);
    async::mysql::setMaxConnection(1);

    async::mysql::execute("192.168.0.81|3306|test|game|game", "select * from mytest2", [](int err, void* row, int row_idx, int, int affected_row) {
        if (err != 0) {
            std::cout << "error:" << err << std::endl;
            //return;
        }
        if (row_idx == 0) {
            std::cout << "no row" << std::endl;
            //return;
        }
        if (row_idx == affected_row) {
            std::cout << "finish" << std::endl;
            return;
        }

        std::cout << ((char**)row)[0] << ", " << ((char**)row)[1] << std::endl;
    });

    async::mysql::execute("192.168.0.81|3306|test|game|game", "select * from mytest", [](int err, void* row, int row_idx, int, int affected_row) {
        if (err != 0) {
            std::cout << "error:" << err << std::endl;
            //return;
        }
        if (row_idx == 0) {
            std::cout << "no row" << std::endl;
            //return;
        }
        if (row_idx != 0) {
            std::cout << ((char**)row)[0] << ", " << ((char**)row)[1] << std::endl;
        }
        if (row_idx == affected_row) {
            std::cout << "finish" << std::endl;
            return;
        }
    });

    while (true) {
        async::loop();
        usleep(1);
    }
}

void co_mysql_test() {
    // 定义一个线程池对象
    ThreadPool tp;
    // 设置mysql的异步驱动由线程池来执行
    async::mysql::setThreadFunc([&tp](std::function<void()> f) {
            tp.enqueue(f);
    });
    sleep(2);
    std::cout << "begin..." << std::endl;

    int count = 0;
    for (int i = 0; i < 100; ++i) {
        // 启动一个协程任务
        CoroutineTask::doTask([i, &count](void*) {
            // 发起mysql访问，并挂起协程，mysql访问结束时，协程被唤醒
            int ret = co_async::mysql::execute("192.168.0.81|3306|test|game|game", "select * from mytest", [i, &count](int err, void* row, int row_idx, int, int affected_row) {
                // 操作结果
                if (err != 0) {
                    std::cout << "error:" << err << std::endl;
                }
                if (row_idx == 1) {
                    std::cout << ".........." << i << "........." << std::endl;
                }
                if (row_idx != 0) {
                    std::cout << ((char**)row)[0] << ", " << ((char**)row)[1] << std::endl;
                }
                if (row_idx == affected_row) {
                    count++;
                    if (count == 100) {
                        std::cout << "finish" << std::endl;
                    }
                }
            });
            // 检查访问状态，成功还是失败或超时
            if (ret != co_bridge::E_CO_RETURN_OK) {
                std::cout << "failed to mysql::execute|" << ret << std::endl;
            }
        }, 0);
    }
    
    while (true) {
        co_bridge::loop();
        usleep(10);
    }
}

#endif