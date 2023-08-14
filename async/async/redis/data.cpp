/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_REDIS

#include "async/async/redis/data.h"
#include <hiredis/hiredis.h>
#include <hiredis/alloc.h>

namespace async {

// 声明
void split(const std::string source, const std::string &separator, std::vector<std::string> &array);

namespace redis {

void freeClusterCore(void*);
void freeNonClusterCore(void*);

RedisAddr::RedisAddr() {}

RedisAddr::RedisAddr(const std::string& id) {
    parse(id);
}

void RedisAddr::parse(const std::string& id) {
    this->id = id;
    
    std::vector<std::string> values;
    async::split(id, "|", values);

    if (values.size() == 5) {
        this->pwd = values[2];
        this->dbIdx = atoi(values[3].c_str());
        this->cluster = (bool)atoi(values[4].c_str());
    
        if (this->cluster) {
            std::vector<std::string> hostVec;
            std::vector<std::string> portVec;
            async::split(values[0], ",", hostVec);
            async::split(values[1], ",", portVec);
            if (hostVec.size() == portVec.size()) {
                std::string host;
                for (size_t idx = 0; idx < hostVec.size(); idx++) {
                    if (!host.empty()) {
                        host += ",";
                    }
                    host += hostVec[idx] + ":" + portVec[idx];
                }    
                this->host = host;
            }
        } else {
            this->host = values[0];
            this->port = atoi(values[1].c_str());
        }
    } else {
        assert(false);
    }
}

RedisConn::~RedisConn() {
    if (this->cluster) {
        freeClusterCore(this->ctxt);
    } else {
        if (this->activeClosed) {
            freeNonClusterCore(this->ctxt);
        }
    }
}

RedisCore::~RedisCore() { 
}

uint32_t CorePool::getTask() {
    uint32_t task = 0;
    for (auto& c : this->coreMap) {
        task += c.second->task;
    }
    return task;
}

void convertCmd(const BaseRedisCmd& cmd, std::vector<const char *>& argv, std::vector<size_t>& argc) {
    for (size_t idx = 0; idx < cmd.cmd.size(); ++idx) {
        if (cmd.cmd[idx].empty()) {
            argv[idx] = 0;
        }
        else {
            argv[idx] = cmd.cmd[idx].c_str();
        }
        argc[idx] = cmd.cmd[idx].length();
    }
}

// 拷贝一份redisreply
redisReply* copyRedisReply(redisReply* reply) {
    if (!reply) {
        return nullptr;
    }

	redisReply* newReply = (redisReply*)hi_calloc(1, sizeof(*reply));
	memcpy(newReply, reply, sizeof(*reply));
    
    //copy str
	if(REDIS_REPLY_ERROR==reply->type 
        || REDIS_REPLY_STRING==reply->type 
        || REDIS_REPLY_STATUS==reply->type)  {
		newReply->str = (char*)hi_malloc(reply->len+1);
		memcpy(newReply->str, reply->str, reply->len);
		newReply->str[reply->len] = '\0';
	}
    //copy array
	else if(REDIS_REPLY_ARRAY==reply->type) {
		newReply->element = (redisReply**)hi_calloc(reply->elements, sizeof(redisReply*));
		memset(newReply->element, 0, newReply->elements*sizeof(redisReply*));
		for(uint32_t i=0; i<reply->elements; ++i) {
			if(NULL!=reply->element[i]) {
				if( NULL == (newReply->element[i] = copyRedisReply(reply->element[i])) ) {
					freeReplyObject(newReply);
					return NULL;
				}
			}
		}
	}

	return newReply;
}

}
}
#endif