/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_REDIS

#include "common/async/redis/redis_parser.h"
#include "common/async/redis/redis_exception.h"
#include <string.h>
#include <assert.h>

#ifndef WIN32
#include <hiredis-vip/hiredis.h>
#else
struct redisReply {
	int type; /* REDIS_REPLY_* */
	long long integer; /* The integer when type is REDIS_REPLY_INTEGER */
	int len; /* Length of string */
	char *str; /* Used for both REDIS_REPLY_ERROR and REDIS_REPLY_STRING */
	size_t elements; /* number of elements, for REDIS_REPLY_ARRAY */
	redisReply **element; /* elements vector for REDIS_REPLY_ARRAY */
};
#endif

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
		if (!(redisReply*)_reply) {
			error = RedisException(M_ERR_REDIS_REPLY_NULL);
			break;
		}
		if (((redisReply*)_reply)->type == REDIS_REPLY_NIL) {
			break;
		}
		if (((redisReply*)_reply)->type == REDIS_REPLY_ERROR) {
			error = RedisException(((redisReply*)_reply)->str);
			break;
		}
		if (((redisReply*)_reply)->type == REDIS_REPLY_SELF_TIMEOUT) {
			error = RedisException(M_ERR_REDIS_CONNECT_FAIL);
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
			assert(false);
		}
		throw error;
	}
}

void RedisReplyParser::GetString(std::string& value) {
	RedisException error;
	do {
		if (!((redisReply*)_reply)) {
			error = RedisException(M_ERR_REDIS_REPLY_NULL);
			break;
		}
		if (((redisReply*)_reply)->type == REDIS_REPLY_NIL) {
			break;
		}
		if (((redisReply*)_reply)->type == REDIS_REPLY_ERROR) {
			error = RedisException(((redisReply*)_reply)->str);
			break;
		}
		if (((redisReply*)_reply)->type == REDIS_REPLY_SELF_TIMEOUT) {
			error = RedisException(M_ERR_REDIS_CONNECT_FAIL);
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
			assert(false);
		}
		throw error;
	}
}

void RedisReplyParser::GetString(char* value, unsigned int len) {
	RedisException error;
	do {
		if (!((redisReply*)_reply)) {
			error = RedisException(M_ERR_REDIS_REPLY_NULL);
			break;
		}
		if (((redisReply*)_reply)->type == REDIS_REPLY_NIL) {
			break;
		}
		if (((redisReply*)_reply)->type == REDIS_REPLY_ERROR) {
			error = RedisException(((redisReply*)_reply)->str);
			break;
		}
		if (((redisReply*)_reply)->type == REDIS_REPLY_SELF_TIMEOUT) {
			error = RedisException(M_ERR_REDIS_CONNECT_FAIL);
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
			assert(false);
		}
		throw error;
	}
}

void RedisReplyParser::GetOk(bool& value) {
	RedisException error;
	do {
		if (!((redisReply*)_reply)) {
			error = RedisException(M_ERR_REDIS_REPLY_NULL);
			break;
		}
		if (((redisReply*)_reply)->type == REDIS_REPLY_NIL) {
			break;
		}
		if (((redisReply*)_reply)->type == REDIS_REPLY_ERROR) {
			error = RedisException(((redisReply*)_reply)->str);
			break;
		}	
		if (((redisReply*)_reply)->type == REDIS_REPLY_SELF_TIMEOUT) {
			error = RedisException(M_ERR_REDIS_CONNECT_FAIL);
			break;
		}	
		if (((redisReply*)_reply)->type != REDIS_REPLY_STATUS) {
			error = RedisException(M_ERR_NOT_DEFINED);
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
			assert(false);
		}
		throw error;
	}
}

///////////////////////////////////////////////////////////////////////////////

void RedisReplyParser::GetValue(void* reply, std::string& value) {
	value.append(((redisReply*)reply)->str, ((redisReply*)reply)->len);
}

template<typename T>
void RedisReplyParser::GetValue(void* reply, T& value) {
	std::istringstream iss(std::string(((redisReply*)reply)->str, ((redisReply*)reply)->len));
	iss >> value;
}

template <typename T>
void RedisReplyParser::GetArray(T &values) {
	RedisException error;
	do {
		if (!((redisReply*)_reply)) {
			error = RedisException(M_ERR_REDIS_REPLY_NULL);
			break;
		}
		if (((redisReply*)_reply)->type == REDIS_REPLY_NIL) {
			break;
		}
		if (((redisReply*)_reply)->type == REDIS_REPLY_ERROR) {
			error = RedisException(((redisReply*)_reply)->str);
			break;
		}
		if (((redisReply*)_reply)->type == REDIS_REPLY_SELF_TIMEOUT) {
			error = RedisException(M_ERR_REDIS_CONNECT_FAIL);
			break;
		}
		if (((redisReply*)_reply)->type != REDIS_REPLY_ARRAY) {
			error = RedisException(M_ERR_NOT_DEFINED);
			break;
		}

		for (size_t idx = 0; idx < ((redisReply*)_reply)->elements; ++idx) {
			redisReply *ele = ((redisReply*)_reply)->element[idx];
			typename T::value_type v;
			DataConstruct(v);
			if (ele->type == REDIS_REPLY_STRING) {
				GetValue(ele, v);
			}
			values.push_back(v);
		}
	} while (false);

	if (!error.Empty()) {
		throw error;
	}
}

template<typename T>
void RedisReplyParser::GetArray(std::set<T>& values) {
	RedisException error;
	do {
		if (!((redisReply*)_reply)) {
			error = RedisException(M_ERR_REDIS_REPLY_NULL);
			break;
		}
		if (((redisReply*)_reply)->type == REDIS_REPLY_NIL) {
			break;
		}
		if (((redisReply*)_reply)->type == REDIS_REPLY_ERROR) {
			error = RedisException(((redisReply*)_reply)->str);
			break;
		}
		if (((redisReply*)_reply)->type == REDIS_REPLY_SELF_TIMEOUT) {
			error = RedisException(M_ERR_REDIS_CONNECT_FAIL);
			break;
		}
		if (((redisReply*)_reply)->type != REDIS_REPLY_ARRAY) {
			error = RedisException(M_ERR_NOT_DEFINED);
			break;
		}

		for (size_t idx = 0; idx < ((redisReply*)_reply)->elements; ++idx) {
			redisReply *ele = ((redisReply*)_reply)->element[idx];
			T v;
			DataConstruct(v);
			if (ele->type == REDIS_REPLY_STRING) {
				GetValue(ele, v);
			}
			values.insert(v);
		}
	} while (false);

	if (!error.Empty()) {
		throw error;
	}
}

template <typename T1, typename T2>
void RedisReplyParser::GetMap(std::map<T1, T2> &values) {
	RedisException error;
	do {
		if (!((redisReply*)_reply)) {
			error = RedisException(M_ERR_REDIS_REPLY_NULL);
			break;
		}
		if (((redisReply*)_reply)->type == REDIS_REPLY_NIL) {
			break;
		}
		if (((redisReply*)_reply)->type == REDIS_REPLY_ERROR) {
			error = RedisException(((redisReply*)_reply)->str);
			break;
		}
		if (((redisReply*)_reply)->type == REDIS_REPLY_SELF_TIMEOUT) {
			error = RedisException(M_ERR_REDIS_CONNECT_FAIL);
			break;
		}
		if (((redisReply*)_reply)->type != REDIS_REPLY_ARRAY) {
			error = RedisException(M_ERR_NOT_DEFINED);
			break;
		}

		for (size_t idx = 0; idx < ((redisReply*)_reply)->elements; idx += 2) {
			T1 value1;
			GetValue(((redisReply*)_reply)->element[idx], value1);

			T2 value2;
			GetValue(((redisReply*)_reply)->element[idx + 1], value2);

			values.insert(std::make_pair(value1, value2));
		}

	} while (false);

	if (!error.Empty()) {
		throw error;
	}
}

template<typename T>
void RedisReplyParser::GetScan(long long& cursor, T& values) {
    RedisException error;
    do {
        if (!((redisReply*)_reply)) {
			error = RedisException(M_ERR_REDIS_REPLY_NULL);
			break;
		}
        if (((redisReply*)_reply)->type == REDIS_REPLY_NIL) {
			break;
		}
		if (((redisReply*)_reply)->type == REDIS_REPLY_ERROR) {
			error = RedisException(((redisReply*)_reply)->str);
			break;
		}
		if (((redisReply*)_reply)->type == REDIS_REPLY_SELF_TIMEOUT) {
			error = RedisException(M_ERR_REDIS_CONNECT_FAIL);
			break;
		}
        if (((redisReply*)_reply)->type != REDIS_REPLY_ARRAY) {
			error = RedisException(M_ERR_NOT_DEFINED);
			break;
		}

        if (((redisReply*)_reply)->elements < 2) {
            error = RedisException(M_ERR_REDIS_SCAN);
			break;
        }

        GetValue(((redisReply*)_reply)->element[0], cursor);
        redisReply* sub_reply = ((redisReply*)_reply)->element[1];
        if (sub_reply->type != REDIS_REPLY_ARRAY) {
            error = RedisException(M_ERR_REDIS_SCAN);
            break;
        }

        for (size_t idx = 0; idx < sub_reply->elements; ++idx) {
            redisReply *ele = sub_reply->element[idx];
			typename T::value_type v;
			DataConstruct(v);
			if (ele->type == REDIS_REPLY_STRING) {
				GetValue(ele, v);
			}
			values.push_back(v);
		}

    } while (false);

    if (!error.Empty()) {
		throw error;
	}
}

} // redis
} // async

#endif