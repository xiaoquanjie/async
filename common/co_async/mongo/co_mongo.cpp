/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_MONGO

#include "common/co_async/mongo/co_mongo.h"
#include "common/coroutine/coroutine.hpp"
#include "common/co_bridge/co_bridge.h"

namespace co_async {
namespace mongo {

int g_wait_time = co_bridge::E_WAIT_FIVE_SECOND;

int getWaitTime() {
    return g_wait_time;
}

void setWaitTime(int wait_time) {
    g_wait_time = wait_time;
}

struct co_mongo_result {
    bool timeout_flag = false;
    async::mongo::MongoReplyParserPtr parser;
};

////////////////////////////////////////////////////////////////////

int execute(const std::string& uri, const async::mongo::BaseMongoCmd& cmd, async::mongo::async_mongo_cb cb) {
    unsigned int co_id = Coroutine::curid();
    if (co_id == M_MAIN_COROUTINE_ID) {
        assert(false);
        return co_bridge::E_CO_RETURN_ERROR;
    }

    int64_t unique_id = co_bridge::gen_unique_id();
    co_mongo_result* result = new co_mongo_result;

    int64_t timer_id = co_bridge::add_timer(g_wait_time, [result, co_id, unique_id]() {
        result->timeout_flag = true;
        co_bridge::rm_unique_id(unique_id);
        Coroutine::resume(co_id);
    });

    async::mongo::execute(uri, cmd, [result, timer_id, co_id, unique_id](async::mongo::MongoReplyParserPtr parser) {
        if (!co_bridge::rm_unique_id(unique_id)) {
            return;
        }
        co_bridge::rm_timer(timer_id);
        result->parser = parser;
        Coroutine::resume(co_id);
    });

    co_bridge::add_unique_id(unique_id);
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

int execute(const std::string& uri, const async::mongo::BaseMongoCmd& cmd) {
    bool ok = true;
    int ret = co_async::mongo::execute(uri, cmd, [&ok](async::mongo::MongoReplyParserPtr parser) {
        if (!parser->IsOk()) {
            printf("failed to execute mongo:%s\n", parser->What());
            ok = false;
        }
    });

    if (!ok) {
        return co_bridge::E_CO_RETURN_ERROR;
    }
    return ret;
}

int execute(const std::string& uri, const async::mongo::BaseMongoCmd& cmd, std::string& json_result) {
    bool ok = true;
    int ret = co_async::mongo::execute(uri, cmd, [&ok, &json_result](async::mongo::MongoReplyParserPtr parser) {
        if (!parser->IsOk()) {
            ok = false;
            printf("failed to execute mongo:%s\n", parser->What());
            return;
        }
        
        json_result.clear();
		json_result.append("[");
		int idx = 0;
		while (true) {
			char* json = parser->NextJson();
			if (!json) {
				break;
			}
			if (idx != 0) {
				json_result.append(",");
			} 
			++idx;
			json_result.append(json);
			parser->FreeJson(json);
		}
    });

    if (!ok) {
        return co_bridge::E_CO_RETURN_ERROR;
    }
    return ret;
}

//int execute(const std::string& uri, const async::mongo::BaseMongoCmd& cmd, int& modifiedOrInsertCount);

//int execute(const std::string& uri, const async::mongo::BaseMongoCmd& cmd, int& modifiedCount, int& upsertedCount);

}
}

#endif