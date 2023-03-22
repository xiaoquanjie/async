/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include <string>
#include <memory>

#ifndef REDIS_ERR
#define REDIS_ERR -1
#endif

#ifndef REDIS_OK
#define REDIS_OK 0
#endif

#ifndef REDIS_REPLY_STRING
#define REDIS_REPLY_STRING 1
#endif

#ifndef REDIS_REPLY_ARRAY
#define REDIS_REPLY_ARRAY 2
#endif

#ifndef REDIS_REPLY_INTEGER
#define REDIS_REPLY_INTEGER 3
#endif

#ifndef REDIS_REPLY_NIL
#define REDIS_REPLY_NIL 4
#endif

#ifndef REDIS_REPLY_STATUS
#define REDIS_REPLY_STATUS 5
#endif

#ifndef REDIS_REPLY_ERROR
#define REDIS_REPLY_ERROR 6
#endif

#ifndef REDIS_REPLY_SELF_TIMEOUT
#define REDIS_REPLY_SELF_TIMEOUT 10000
#endif

#define M_ERR_NOT_DEFINED			("wrong type")
#define M_ERR_REDIS_CONNECT_FAIL	("redisConnect fail")
#define M_ERR_REDIS_NOT_CONNECTED	("redis is not connected")
#define M_ERR_REDIS_REPLY_NULL		("redis reply is null")
#define M_ERR_REDIS_TYPE_NOT_MATCH	("type doesn't match")
#define M_ERR_REDIS_KEY_NOT_EXIST	("key not exist")
#define M_ERR_REDIS_ARRAY_SIZE_NOT_MATCH ("array size doesn't match")
#define M_ERR_REDIS_SELECT_DB_ERROR ("select db error")
#define M_ERR_REDIS_AUTH_ERROR		("auth error")	
#define M_ERR_REDIS_SCAN            ("scan error")
#define M_ERR_REDIS_TIMEOUT         ("request timeout")


namespace async {
namespace redis {

// redis异常
struct RedisException
{
	RedisException();

	RedisException(const char* what);

	RedisException(const std::string& what);

	const std::string& What() const;

	bool Empty() const;

private:
	std::shared_ptr<std::string> m_what;
};

}
}