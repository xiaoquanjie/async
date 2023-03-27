#ifdef USE_ASYNC_MONGO

#include "./common.h"

extern bool print_log;

// mongo测试
void mongo_test(bool use_co) {
    std::string uri = "mongodb://127.0.0.1:27017|test|student2";
    // 使用协程
    if (use_co) {
        // 启动一个协程任务
        CoroutineTask::doTask([uri](void*) {
            {
                auto res = co_async::mongo::execute(uri, async::mongo::CreateExpireIndexMongoCmd("name", 120));
                if (res.second) {
                    log("ok");
                } else {
                    log("fail");
                }
                return;
            }

            // 执行mongo访问并等待协程返回
            {
                // ok
                std::string value;
                co_async::mongo::execute(uri, async::mongo::FindMongoCmd({}), &value);
                std::cout << "ret1:" << value << std::endl;
            }

            {
                int modifiedCount = 0;
                co_async::mongo::execute(uri, async::mongo::UpdateManyMongoCmd({
                    {"userName", "test3"}
                }, {
                    {"password", "1234510"}
                }), &modifiedCount, 0, 0);
                
                std::cout << "ret4:" << modifiedCount << std::endl;
            }
            
        }, 0);
    }
    // 不使用协程
    else {
        // 发起一个mongo异步访问并期待回调被调用
        async::mongo::execute(uri, async::mongo::FindMongoCmd({}), [](async::mongo::MongoReplyParserPtr ptr) {
            while (true) {
                char* json = ptr->NextJson();
                if (!json) {
                    break;
                }
                std::cout << "mongo value:" << json << std::endl;
                ptr->FreeJson(json);
            }
        });
    }
}


#endif