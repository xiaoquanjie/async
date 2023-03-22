/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_REDIS

#include "common/async/redis/redis_exception.h"

namespace async {
namespace redis {

RedisException::RedisException() {
}

RedisException::RedisException(const char* what) {
    m_what = std::make_shared<std::string>(what);
}

RedisException::RedisException(const std::string& what) {
	m_what = std::make_shared<std::string>(what);
}

const std::string& RedisException::What()const {
	if (!m_what) {
        static std::string empty;
        return empty;
    } else {
        return *m_what;
    }
}

bool RedisException::Empty()const {
	return (!m_what);
}

} // redis
} // async

#endif