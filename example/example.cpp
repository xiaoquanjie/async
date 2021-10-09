#include "common/async/async.h"
#include "common/co_async/co_async.hpp"
#include "common/coroutine/coroutine.hpp"
#include "common/threads/thread_pool.h"
#include <sys/time.h>
#include <unistd.h>
#include <iostream>

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
void redis_test(bool use_co, int& count, TimeElapsed& te, const char* url) {
    if (use_co) {
        CoroutineTask::doTask([&count, &te, url](void*) {
            static int fail_count = 0;
            static int timeout_count = 0;
            static int success_count = 0;

            std::string value;
            int ret = co_async::redis::execute(url, async::redis::GetRedisCmd("mytest"), value);
            if (ret == co_async::E_CO_RETURN_TIMEOUT) {
                timeout_count++;
            }
            else if (ret == co_async::E_CO_RETURN_ERROR) {
                fail_count++;
            }
            else if (ret == co_async::E_CO_RETURN_OK) {
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
    else {
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

void mongo_test(bool use_co, int& count, TimeElapsed& te, const char* url) {
    if (use_co) {
        CoroutineTask::doTask([&count, &te, url](void*) {
            static int fail_count = 0;
            static int timeout_count = 0;
            static int success_count = 0;

            std::string value;
            int ret = co_async::mongo::execute(url, async::mongo::FindMongoCmd({}), value);
            if (ret == co_async::E_CO_RETURN_TIMEOUT) {
                timeout_count++;
            }
            else if (ret == co_async::E_CO_RETURN_ERROR) {
                fail_count++;
            }
            else if (ret == co_async::E_CO_RETURN_OK) {
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
    else {
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

void curl_test(bool use_co, int& count, TimeElapsed& te, const char* url) {
    if (use_co) {
        CoroutineTask::doTask([&count, &te, url](void*) {
            static int fail_count = 0;
            static int timeout_count = 0;
            static int success_count = 0;

            std::string body;
            int ret = co_async::curl::get(url, [&count, &te, &body](int, int, std::string& b) {
                body.swap(b);
            });
            if (ret == co_async::E_CO_RETURN_TIMEOUT) {
                timeout_count++;
            }
            else if (ret == co_async::E_CO_RETURN_ERROR) {
                fail_count++;
            }
            else if (ret == co_async::E_CO_RETURN_OK) {
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
    else {
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
    if (use_co) {
        CoroutineTask::doTask([&count, &te](void*) {
            static int fail_count = 0;
            static int timeout_count = 0;
            static int success_count = 0;

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

            if (ret == co_async::E_CO_RETURN_TIMEOUT) {
                timeout_count++;
            }
            else if (ret == co_async::E_CO_RETURN_ERROR) {
                fail_count++;
            }
            else if (ret == co_async::E_CO_RETURN_OK) {
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
    else {
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
    if (use)
    {
        async::curl::set_thread_func([&tp](std::function<void()> f) { 
            tp.enqueue(f); 
        });
        async::redis::set_thread_func([&tp](std::function<void()> f) { 
            tp.enqueue(f); 
        });
        async::mongo::set_thread_func([&tp](std::function<void()> f) {
            tp.enqueue(f); 
        });
        async::cpu::set_thread_func([&tp](std::function<void()> f) {
            tp.enqueue(f);
        });
        sleep(2);
    }

    co_async::redis::set_wait_time(20 * 1000);

    const char* url = 0;
    const char* mongo_url = 0;
    const char* http_url = 0;
    std::cout << "use right uri:" << std::endl;
    std::cin >> use;
    if (use) {
        url = "192.168.0.88:6379::0:0";
        mongo_url = "mongodb://192.168.0.31:27017|test|cash_coupon_whitelist||";
        http_url = "http://www.baidu.com";
    }
    else {
        url = "192.168.0.88:6389::0:0";
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
            if (co_async::loop()) {
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
    CoroutineTask::doTask([](void*) {
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
        co_async::loop();
        sleep(1);
    }
    return 0;
}

int main6() {
    async::mysql::set_keep_connection(1);
    async::mysql::set_max_connection(1);

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

int main() {
    ThreadPool tp;
    async::mysql::set_thread_func([&tp](std::function<void()> f) {
            tp.enqueue(f);
    });
    sleep(2);
    std::cout << "begin..." << std::endl;

    int count = 0;
    for (int i = 0; i < 100; ++i) {
        CoroutineTask::doTask([i, &count](void*) {
            int ret = co_async::mysql::execute("192.168.0.81|3306|test|game|game", "select * from mytest", [i, &count](int err, void* row, int row_idx, int, int affected_row) {
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
            if (ret != co_async::E_CO_RETURN_OK) {
                std::cout << "failed to mysql::execute|" << ret << std::endl;
            }
        }, 0);
    }
    
    while (true) {
        co_async::loop();
        usleep(10);
    }
    return 0;
}