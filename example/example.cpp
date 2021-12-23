#include "common/async/async.h"
#include "common/co_bridge/co_bridge.h"
#include "common/coroutine/coroutine.hpp"
#include "common/threads/thread_pool.h"
#include "common/ipc/zero_mq_handler.h"
#include "common/transaction/transaction_mgr.h"
#include <sys/time.h>
#include <unistd.h>
#include <iostream>

// 计时器
struct TimeElapsed {
    void begin() {
        gettimeofday(&tv, NULL);
    }

    uint64_t end() {
        struct timeval tv2;
        gettimeofday(&tv2, NULL);
	    return (tv2.tv_sec * 1000 + tv2.tv_usec / 1000) - (tv.tv_sec * 1000 + tv.tv_usec / 1000);
    }

    struct timeval tv;
};

////////////////////
bool print_log = false;

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
            int ret = co_async::mongo::execute(url, async::mongo::FindMongoCmd({}), value);
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

int main3() {
    std::cout << "use thread pool:" << std::endl;
    int use = 0;
    std::cin >> use;
    ThreadPool tp;
    // 使用线程池
    if (use)
    {
        async::curl::setThreadFunc([&tp](std::function<void()> f) { 
            tp.enqueue(f); 
        });
        async::redis::setThreadFunc([&tp](std::function<void()> f) { 
            tp.enqueue(f); 
        });
        async::mongo::setThreadFunc([&tp](std::function<void()> f) {
            tp.enqueue(f); 
        });
        async::cpu::setThreadFunc([&tp](std::function<void()> f) {
            tp.enqueue(f);
        });
        sleep(2);
    }

    co_async::redis::setWaitTime(20 * 1000);

    const char* url = 0;
    const char* mongo_url = 0;
    const char* http_url = 0;
    std::cout << "use right uri:" << std::endl;
    std::cin >> use;
    if (use) {
        url = "192.168.0.88|6379||0|0";
        mongo_url = "mongodb://192.168.0.31:27017|test|cash_coupon_whitelist||";
        http_url = "http://www.baidu.com";
    }
    else {
        url = "192.168.0.88|6389||0|0";
        mongo_url = "mongodb://192.168.0.30:27018|test|cash_coupon_whitelist||";
        http_url = "http://www.baidu.com2";
    }

    int use_co = 0;
    std::cout << "use coroutine:" << std::endl;
    std::cin >> use_co;
    
    if (use_co) {
        Coroutine::init();
    }

    std::cout << "print_log:" << std::endl;
    std::cin >> print_log;

    int total_count = 1000;
    std::cout << "run count:" << std::endl;
    std::cin >> total_count;

    int cpu_count = total_count;
    int redis_count = total_count;
    int mongo_count = total_count;
    int curl_count = total_count;

    if (url || mongo_url || http_url || cpu_count ||redis_count|| mongo_count ||curl_count) {}

    TimeElapsed te;
    te.begin();
    int idle_cnt = 0;
    while (true) {
        if (total_count > 0) {
            redis_test(use_co, redis_count, te, url);
            mongo_test(use_co, mongo_count, te, mongo_url);
            cpu_test(use_co, cpu_count, te);
            curl_test(use_co, curl_count, te, http_url);
            total_count--;
        }

        if (redis_count == 0
            && mongo_count == 0
            && cpu_count == 0
            && curl_count == 0) {
            std::cout << "elapsed:" << te.end() << std::endl;
            break;
        }

        if (use_co) {
            if (co_bridge::loop()) {
                idle_cnt = 0;
            }
            else {
                idle_cnt++;
            }
        }
        else {
            if (async::loop()) {
                idle_cnt = 0;
            }
            else {
                idle_cnt++;
            }
        }

        if (idle_cnt > 500) {
            usleep(5);
            idle_cnt = 0;
        }
    }

    std::cout << "over" << std::endl;
    return 0;   
}

int main4() {
    // 异步io并行访问，并期待结束时回调被调用
    async::parallel([](int err) {
        std::cout << "parallel done:" << err << std::endl;
    },
    {
        [](async::next_cb next) {
            async::curl::get("http://baidu.com", [next](int, int, std::string& body) {
                std::cout << "parallel body1:" << body.length() <<std::endl;
                next(0);
            });
        },
        [](async::next_cb next) {
            async::curl::get("http://baidu.com", [next](int, int, std::string& body) {
                std::cout << "parallel body2:" << body.length() <<std::endl;
                next(2);
            });
        }
    });

    // 异步io串行访问，并期待结束时回调被调用
    async::series([](int err) {
        std::cout << "series done:" << err << std::endl;
    },
    {
        [](async::next_cb next) {
            async::curl::get("http://baidu.com", [next](int, int, std::string& body) {
                std::cout << "series body3:" << body.length() <<std::endl;
                next(0);
            });
        },
        [](async::next_cb next) {
            async::curl::get("http://baidu.com", [next](int, int, std::string& body) {
                std::cout << "series body4:" << body.length() <<std::endl;
                next(4);
            });
        }
    });

    while (true) {
        async::loop();
        sleep(1);
    }
    return 0;
}

int main5() {
    // 启动一个协程任务
    CoroutineTask::doTask([](void*) {
        // 异步io并行访问，等待访问结束时协程被唤醒
        int ret = co_async::parallel(
        {
            [](co_async::next_cb next) {
                async::curl::get("http://baidu.com", [next](int, int, std::string& body) {
                    std::cout << "parallel body3:" << body.length() <<std::endl;
                    next();
                });
            },
            [](co_async::next_cb next) {
                async::curl::get("http://192.168.0.22:5003", [next](int, int, std::string& body) {
                    std::cout << "parallel body4:" << body.length() <<std::endl;
                    next();
                });
            }
        });
        std::cout << "parallel over" << ret << std::endl;
    }, 0);

    CoroutineTask::doTask([](void*) {
        int ret = co_async::series(
        {
            [](co_async::next_cb next) {
                async::curl::get("http://baidu.com", [next](int, int, std::string& body) {
                    std::cout << "series body1:" << body.length() <<std::endl;
                    next();
                });
            },
            [](co_async::next_cb next) {
                async::curl::get("http://192.168.0.22:5003", [next](int, int, std::string& body) {
                    std::cout << "series body2:" << body.length() <<std::endl;
                    next();
                });
            }
        });
        std::cout << "series over" << ret << std::endl;
    }, 0);

    while (true) {
        co_bridge::loop();
        sleep(1);
    }
    return 0;
}

int main6() {
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
    return 0;
}

int main7() {
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
    return 0;
}

///////////////////////////////////////////////////////////////////

class RouterHandler : public ZeromqRouterHandler {
public:

protected:
    void onData(uint64_t unique_id, std::string& identify, const std::string& data)  {
        std::cout << "router:" << unique_id << "|" << identify <<"|" << data << std::endl;
        std::string rsp_data = ("router resoond " + data);
        this->sendData(unique_id, identify, rsp_data);
    }
};

class DealerHandler : public ZeromqDealerHandler {
public:

protected:
    void onData(uint64_t unique_id, std::string& identify, const std::string& data)  {
        std::cout << "dealer:" << unique_id << "|" << identify <<"|" << data << std::endl;
    }
};


int main8() {
    RouterHandler routerHandler;
    DealerHandler dealerHandler;
    routerHandler.listen(1, "tcp://0.0.0.0:1000");
    routerHandler.listen(2, "tcp://0.0.0.0:1001");
    int id1 = dealerHandler.connect(1, "tcp://127.0.0.1:1000"); 
    int id2 = dealerHandler.connect(2, "tcp://127.0.0.1:1001"); 
    dealerHandler.sendData(id2, "hello world");
    dealerHandler.sendData(id1, "hello world2");
    while (true) {
        routerHandler.update(0);
        dealerHandler.update(0);
        usleep(1);
    }
    return 0;
}

///////////////////////////////////////////////////////////////////

// 模拟消息id
#define CmdTestStructReq (1)
#define CmdTestStructRsp (2)

struct TestStructReq {
    int id = 1;
};

struct TestStructRsp {
    int ret = 2;
};

void operator >> (std::istringstream& iss, TestStructReq& s) {
    iss >> s.id;
}

// 实现一个带解析器的ServerTransaction
template<typename RequestType, typename RespondType = NullRespond>
class ServerTransaction : public Transaction<RequestType, RespondType> {
protected:
    // 实现包解析器
    virtual bool ParsePacket(const char* packet, uint32_t packet_size) override {
        std::istringstream iss(std::string(packet, packet_size));
        iss >> Transaction<RequestType, RespondType>::m_request;
        return true;
    }

    virtual bool BeforeOnRequest() override {
        std::cout << "before" << std::endl;
        return true;
    }

    virtual bool AfterOnRequest() override {
        std::cout << "after, 回复数据" << std::endl;
        return true;
    }
};

// 实现一个带解析器的ServerTransaction
template<typename RequestType>
class ServerTransaction<RequestType, NullRespond> : public Transaction<RequestType> {
protected:
    // 实现包解析器
    virtual bool ParsePacket(const char* packet, uint32_t packet_size) override {
        std::istringstream iss(std::string(packet, packet_size));
        iss >> Transaction<RequestType>::m_request;
        return true;
    }

    virtual void BeforeOnRequest() override {
        std::cout << "before" << std::endl;
    }

    virtual void AfterOnRequest() override {
        std::cout << "after" << std::endl;
    }
};

// 定义一个事务
class TestStructTransaction : public ServerTransaction<TestStructReq, TestStructRsp> {
public:
    int OnRequest() override {
        std::cout << "data:" << m_request.id << std::endl;
        m_respond.ret = 3;
        std::cout << "co_id:" << Coroutine::curid() << std::endl;

        // 访问redis
        std::string value;
        co_async::redis::execute("192.168.0.88|6379||0|0", async::redis::GetRedisCmd("mytest"), value);
        std::cout << "redis value:" << value << std::endl;
        return 0;
    }
};

// 注册
REGIST_TRANSACTION(TestStruct);

class MyTickTransaction : public BaseTickTransaction {
public:
    void OnTick(uint32_t) {
        // 访问redis
        //std::string value;
        //co_async::redis::execute("192.168.0.88|6379||0|0", async::redis::GetRedisCmd("mytest"), value);
        //std::cout << "tick redis value:" << value << std::endl;
    }
};

REGIST_TICK_TRANSACTION(MyTickTransaction);

int main() {
    // 模拟数据包
    std::ostringstream oss;
    oss << 100;
    trans_mgr::handle(CmdTestStructReq, oss.str().c_str(), oss.str().length());

    while (true) {
        co_bridge::loop();
        usleep(2000);
        trans_mgr::tick(time(0));
    }
    return 0;
}