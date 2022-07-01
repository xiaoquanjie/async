#ifdef USE_ASYNC_MYSQL

#include "./common.h"

void co_mysql_test()
{
    // 启动一个协程任务
    CoroutineTask::doTask([](void*) {
        {
            auto ret = co_async::mysql::query("192.168.0.89|3306|test||", 
            "select * from student", 
            [](int err, const void* row, int row_idx, int field, int affected_row) {
                    for (int f = 0; f < field; ++f) {
                        std::cout << ((const char**)row)[f] << ", ";
                    }
                    std::cout << std::endl;
            });

            std::cout << "ret:" << ret.first << "|" << ret.second;
        }

        {
            auto ret = co_async::mysql::query("192.168.0.89|3306|test||", "select * from student");
        }
    }, 0);
    
}

#endif