/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_REDIS

#include "common/async/redis/redis_parser.h"
#include "common/async/redis/redis_exception.h"
#include <assert.h>
#include <hiredis/hiredis.h>

namespace async {
namespace redis {

RedisReplyParser::RedisReplyParser() {
	_reply = 0;
}

RedisReplyParser::RedisReplyParser(void* reply) {
	_reply = reply;
}

RedisReplyParser::~RedisReplyParser() {
	if (_reply) {
		freeReplyObject((redisReply*)_reply);
	}
}

bool RedisReplyParser::IsEmpty() const {
	return (_reply == 0);
}

bool RedisReplyParser::IsNil() const {
	if (!_reply) {
		return false;
	}

	return (((redisReply*)_reply)->type == REDIS_REPLY_NIL);
}

bool RedisReplyParser::GetError(std::string& value) {
	if (!_reply) {
		value = M_ERR_REDIS_REPLY_NULL;
	}
	else if (((redisReply*)_reply)->type == REDIS_REPLY_ERROR) {
		value = ((redisReply*)_reply)->str;
	}
	return !value.empty();
}

void RedisReplyParser::GetInteger(long long& value) {
	RedisException error;
	do {
		error = checkReply(_reply, 0);
		if (!error.Empty()) {
			break;
		}

		if (((redisReply*)_reply)->type == REDIS_REPLY_INTEGER) {
			value = ((redisReply*)_reply)->integer;
			break;
		}
		if (((redisReply*)_reply)->type == REDIS_REPLY_STRING) {
			std::istringstream iss(std::string(((redisReply*)_reply)->str, ((redisReply*)_reply)->len));
			iss >> value;
			break;
		}

		error = RedisException(M_ERR_NOT_DEFINED);
	} while (false);

	if (!error.Empty()) {
		if (error.What() == M_ERR_NOT_DEFINED) {
			//assert(false);
		}
		throw error;
	}
}

void RedisReplyParser::GetString(std::string& value) {
	RedisException error;
	do {
		error = checkReply(_reply, 0);
		if (!error.Empty()) {
			break;
		}

		if (((redisReply*)_reply)->type == REDIS_REPLY_STRING) {
			value.clear();
			std::string tmp(((redisReply*)_reply)->str, ((redisReply*)_reply)->len);
			value.swap(tmp);
			//value.append(_reply->str, _reply->len);
			break;
		}
		if (((redisReply*)_reply)->type == REDIS_REPLY_INTEGER) {
			value = std::to_string(((redisReply*)_reply)->integer);
			break;
		}
		error = RedisException(M_ERR_NOT_DEFINED);
	} while (false);

	if (!error.Empty()) {
		if (error.What() == M_ERR_NOT_DEFINED) {
			//assert(false);
		}
		throw error;
	}
}

void RedisReplyParser::GetString(char* value, unsigned int len) {
	RedisException error;
	do {
		error = checkReply(_reply, 0);
		if (!error.Empty()) {
			break;
		}

		if (((redisReply*)_reply)->type == REDIS_REPLY_STRING) {
			if (len > (unsigned int)((redisReply*)_reply)->len) {
				len = (unsigned int)((redisReply*)_reply)->len;
			}
			memcpy(value, ((redisReply*)_reply)->str, len);
			break;
		}
		if (((redisReply*)_reply)->type == REDIS_REPLY_INTEGER) {
			auto s = std::to_string(((redisReply*)_reply)->integer);
			if (len > (unsigned int)s.size()) {
				len = (unsigned int)s.size();
			}
			memcpy(value, s.c_str(), len);
			break;
		}

		error = RedisException(M_ERR_NOT_DEFINED);
	} while (false);

	if (!error.Empty()) {
		if (error.What() == M_ERR_NOT_DEFINED) {
			//assert(false);
		}
		throw error;
	}
}

void RedisReplyParser::GetOk(bool& value) {
	RedisException error;
	do {
		error = checkReply(_reply, REDIS_REPLY_STATUS);
		if (!error.Empty()) {
			break;
		}

		if (strcmp(((redisReply*)_reply)->str, "OK") != 0) {
			value = false;
		}
		else {
			value = true;
		}
	} while (false);

	if (!error.Empty()) {
		if (error.What() == M_ERR_NOT_DEFINED) {
			//assert(false);
		}
		throw error;
	}
}

///////////////////////////////////////////////////////////////////////////////

bool RedisReplyParser::GetValue(void* reply, std::string& value) {
	if (((redisReply*)reply)->type == REDIS_REPLY_STRING) {
		value.append(((redisReply*)reply)->str, ((redisReply*)reply)->len);
		return true;
	}
	return false;
}

RedisException checkReply(void* reply, int hope) {
	RedisException error;
	do {
		if (!((redisReply*)reply)) {
			error = RedisException(M_ERR_REDIS_REPLY_NULL);
			break;
		}
		if (((redisReply*)reply)->type == REDIS_REPLY_NIL) {
			break;
		}
		if (((redisReply*)reply)->type == REDIS_REPLY_ERROR) {
			error = RedisException(((redisReply*)reply)->str);
			break;
		}
		if (((redisReply*)reply)->type == REDIS_REPLY_SELF_TIMEOUT) {
			error = RedisException(M_ERR_REDIS_TIMEOUT);
			break;
		}
		if (hope != 0 && ((redisReply*)reply)->type != hope) {
			error = RedisException(M_ERR_NOT_DEFINED);
			break;
		}
		
	} while (false);

	return error;
}

void iteratorReply(void* reply, std::function<void(void*)> f) {
	if (((redisReply *)reply)->type != REDIS_REPLY_ARRAY) {
		return;
	}
	for (size_t idx = 0; idx < ((redisReply *)reply)->elements; ++idx) {
		redisReply *ele = ((redisReply *)reply)->element[idx];
		f(ele);
	}
}

void iteratorReply(void* reply, std::function<void(void*, void*)> f) {
	if (((redisReply *)reply)->type != REDIS_REPLY_ARRAY) {
		return;
	}
	for (size_t idx = 0; idx < ((redisReply*)reply)->elements; idx += 2) {
		redisReply *ele = ((redisReply *)reply)->element[idx];
		redisReply *ele2 = ((redisReply *)reply)->element[idx + 1];
		f(ele, ele2);
	}
}

void iteratorReply(void* reply, void** ele1, void** ele2) {
	if (((redisReply*)reply)->elements == 2) {
		*ele1 = ((redisReply*)reply)->element[0];
		*ele2 = ((redisReply*)reply)->element[1];
    }
}

} // redis
} // async

#endif