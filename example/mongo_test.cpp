#ifdef USE_ASYNC_MONGO

#include "./common.h"

extern bool print_log;

// mongo测试
void mongo_test(bool use_co) {
    // 使用协程
    if (use_co) {
        // 启动一个协程任务
        CoroutineTask::doTask([](void*) {
            // 执行mongo访问并等待协程返回
            {
                // ok
                std::string value;
                co_async::mongo::execute("mongodb://192.168.0.31:27017|test3|user||", async::mongo::FindMongoCmd({}), &value);
                std::cout << "ret1:" << value << std::endl;
            }
            {
                // fail
                std::string value;
                co_async::mongo::execute("mongodb://192.168.0.31:27|test3|user||", async::mongo::FindMongoCmd({}), &value);
                std::cout << "ret2:" << value << std::endl;
            }

            {
                /*auto res =*/ co_async::mongo::execute("mongodb://192.168.0.31:27017|test3|user||", async::mongo::InsertMongoCmd({
                    {"userName", "test3"},
                    {"password", "1234"}
                }), (std::string*)0);
                //std::cout << "ret3:" << res.second << std::endl;
            }

            {
                int modifiedCount = 0;
                co_async::mongo::execute("mongodb://192.168.0.31:27017|test3|user||", async::mongo::UpdateManyMongoCmd({
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
        async::mongo::execute("mongodb://192.168.0.31:27017|test3|user||", async::mongo::FindMongoCmd({}), [](async::mongo::MongoReplyParserPtr ptr) {
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