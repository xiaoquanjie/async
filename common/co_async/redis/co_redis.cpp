/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_REDIS

#include "common/co_async/redis/co_redis.h"
#include "common/coroutine/coroutine.hpp"
#include "common/async/redis/redis_exception.h"
#include "common/co_bridge/co_bridge.h"

namespace co_async {
namespace redis {

int g_wait_time = co_bridge::E_WAIT_THIRTY_SECOND;

int getWaitTime() {
    return g_wait_time;
}

void setWaitTime(int wait_time) {
    g_wait_time = wait_time;
}

struct co_redis_result {
    bool timeout_flag = false;
    async::redis::RedisReplyParserPtr parser;
};

/////////////////////////////////////////////////////////////////////////

int execute(const std::string& uri, 
    const async::redis::BaseRedisCmd& cmd, 
    async::redis::async_redis_cb cb) {
    unsigned int co_id = Coroutine::curid();
    if (co_id == M_MAIN_COROUTINE_ID) {
        assert(false);
        return co_bridge::E_CO_RETURN_ERROR;
    }

    co_redis_result* result = new co_redis_result;
    int64_t unique_id = co_bridge::genUniqueId();
   
    int64_t timer_id = co_bridge::addTimer(g_wait_time, [result, co_id, unique_id]() {
        result->timeout_flag = true;
        co_bridge::rmUniqueId(unique_id);
        Coroutine::resume(co_id);
    });

    async::redis::execute(uri, cmd, [result, timer_id, co_id, unique_id](async::redis::RedisReplyParserPtr parser) {
        if (!co_bridge::rmUniqueId(unique_id)) {
            return;
        }
        co_bridge::rmTimer(timer_id);
        result->parser = parser;
        Coroutine::resume(co_id);
    });

    co_bridge::addUniqueId(unique_id);
    Coroutine::yield();
    
    int ret = co_bridge::E_CO_RETURN_OK;
    if (result->timeout_flag) {
        ret = co_bridge::E_CO_RETURN_TIMEOUT;
    }
    else {
        cb(result->parser);
    }
    
    delete result;
    return ret;
}

int execute(const std::string& uri, 
    const async::redis::BaseRedisCmd& cmd) {
    std::string result;
    int ret = co_async::redis::execute(uri, cmd, [&result](async::redis::RedisReplyParserPtr parser) {
        parser->GetError(result);
    });

    if (result.size()) {
        printf("failed to execute redis|%s|%s\n", cmd.GetCmd().c_str(), result.c_str());
        return co_bridge::E_CO_RETURN_ERROR;
    }
    return ret;
}

int execute(const std::string& uri, 
    const async::redis::BaseRedisCmd& cmd, 
    long long& result) {
    bool ok = true;
    int ret = co_async::redis::execute(uri, cmd, [&result, &ok, &cmd](async::redis::RedisReplyParserPtr parser) {
        try {
            parser->GetInteger(result);
        }
        catch(async::redis::RedisException& exce) {
            ok = false;
            printf("failed to execute redis|%s|%s\n", cmd.GetCmd().c_str(), exce.What().c_str());
        }
    });

    if (!ok) {
        return co_bridge::E_CO_RETURN_ERROR;
    }
    return ret;
}

int execute(const std::string& uri, 
    const async::redis::BaseRedisCmd& cmd, 
    std::string& result) {
    bool ok = true;
    int ret = co_async::redis::execute(uri, cmd, [&result, &cmd, &ok](async::redis::RedisReplyParserPtr parser) {
        try {
			parser->GetString(result);
		}
		catch (const async::redis::RedisException &exce) {
            ok = false;
			printf("failed to execute redis|%s|%s\n", cmd.GetCmd().c_str(), exce.What().c_str());
		}
    });

    if (!ok) {
        return co_bridge::E_CO_RETURN_ERROR;
    }
    return ret;
}

int execute(const std::string& uri, 
    const async::redis::BaseRedisCmd& cmd, 
    bool& result) {
    bool ok = true;
    int ret = co_async::redis::execute(uri, cmd, [&result, &cmd, &ok](async::redis::RedisReplyParserPtr parser) {
        try {
			parser->GetOk(result);
		}
		catch (const async::redis::RedisException &exce) {
            ok = false;
			printf("failed to execute redis|%s|%s\n", cmd.GetCmd().c_str(), exce.What().c_str());
		}
    });

    if (!ok) {
        return co_bridge::E_CO_RETURN_ERROR;
    }
    return ret;
}

// get array
template<typename T>
int execute(const std::string& uri, 
    const async::redis::BaseRedisCmd& cmd, 
    T& result) {
    bool ok = true;
    int ret = co_async::redis::execute(uri, cmd, [&result, &cmd, &ok](async::redis::RedisReplyParserPtr parser) {
        try {
			parser->GetArray(result);
		}
		catch (const async::redis::RedisException &exce) {
            ok = false;
			printf("failed to execute redis|%s|%s\n", cmd.GetCmd().c_str(), exce.What().c_str());
		}
    });

    if (!ok) {
        return co_bridge::E_CO_RETURN_ERROR;
    }
    return ret;    
}

// get map
template<typename T1, typename T2>
int execute(const std::string& uri, 
    const async::redis::BaseRedisCmd& cmd, 
    std::map<T1, T2>& result) {
    bool ok = true;
    int ret = co_async::redis::execute(uri, cmd, [&result, &cmd, &ok](async::redis::RedisReplyParserPtr parser) {
        try {
			parser->GetMap(result);
		}
		catch (const async::redis::RedisException &exce) {
            ok = false;
			printf("failed to execute redis|%s|%s\n", cmd.GetCmd().c_str(), exce.What().c_str());
		}
    });

    if (!ok) {
        return co_bridge::E_CO_RETURN_ERROR;
    }
    return ret;
}

// get scan result
template<typename T>
int execute(const std::string& uri, 
    const async::redis::BaseRedisCmd& cmd, 
    long long& cursor, 
    T& result) {
    bool ok = true;
    int ret = co_async::redis::execute(uri, cmd, [&result, &cmd, &ok, &cursor](async::redis::RedisReplyParserPtr parser) {
        try {
			parser->GetScan(cursor, result);
		}
		catch (const async::redis::RedisException &exce) {
            ok = false;
			printf("failed to execute redis|%s|%s\n", cmd.GetCmd().c_str(), exce.What().c_str());
		}
    });

    if (!ok) {
        return co_bridge::E_CO_RETURN_ERROR;
    }
    return ret;
}

}
}

#endif