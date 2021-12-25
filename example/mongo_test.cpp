#ifdef USE_ASYNC_MONGO

#include "./common.h"

extern bool print_log;

// mongo测试
void mongo_test(bool use_co, int& count, TimeElapsed& te, const char* url) {
    // 使用协程
    if (use_co) {
        // 启动一个协程任务
        CoroutineTask::doTask([&count, &te, url](void*) {
            static int fail_count = 0;
            static int timeout_count = 0;
            static int success_count = 0;

            // 执行mongo访问并等待协程返回
            std::string value;
            int ret = co_async::mongo::execute(url, async::mongo::FindMongoCmd({{"user_id", 400024, "$gt"}}), value);
            if (ret == co_bridge::E_CO_RETURN_TIMEOUT) {
                timeout_count++;
            }
            else if (ret == co_bridge::E_CO_RETURN_ERROR) {
                fail_count++;
            }
            else if (ret == co_bridge::E_CO_RETURN_OK) {
                success_count++;
                if (print_log) {
                    std::cout << "mongo value:" << value << std::endl;
                }
            }

			count--;
            if (count == 0) {
                std::cout << "mongo elapsed: " << te.end() 
                          << ", success_count:" << success_count 
                          << ", timeout_count:" << timeout_count 
                          << ", fail_count:" << fail_count << std::endl;
            }
        }, 0);
    }
    // 不使用协程
    else {
        // 发起一个mongo异步访问并期待回调被调用
        async::mongo::execute(url, async::mongo::FindMongoCmd({}), [&count, &te](async::mongo::MongoReplyParserPtr ptr) {
            count--;
            while (true) {
                char* json = ptr->NextJson();
                if (!json) {
                    break;
                }
                if (print_log) {
                    std::cout << "mongo value:" << json << std::endl;
                }
                ptr->FreeJson(json);
            }
            if (count == 0) {
                std::cout << "mongo elapsed: " << te.end() << std::endl;
            }
        });
    }
}

void mongo_test() {
    TimeElapsed te;
    te.begin();

    int count = 1;
    const char* mongo_url = "mongodb://192.168.0.31:27017|test|cash_coupon_whitelist||";
    mongo_test(true, count, te, mongo_url);

    while (true) {
        co_bridge::loop(time(0));
        usleep(10);
    }
}

#endif