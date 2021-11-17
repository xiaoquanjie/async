/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include <sstream>
#include <map>
#include <set>
#include <memory>
#include "common/async/redis/redis_exception.h"

namespace async {
namespace redis {

template<typename T>
void DataConstruct(T& v) {
	if (typeid(v) != typeid(std::string)) {
		memset(&v, 0, sizeof(0));
	}
}

RedisException checkReply(void* reply, int hope);

// 迭代
void iteratorReply(void* reply, std::function<void(void*)> f);
void iteratorReply(void* reply, std::function<void(void*, void*)> f);
void iteratorReply(void* reply, void** ele1, void** ele2);

// redis结果解析辅助
struct RedisReplyParser {
	RedisReplyParser();

	RedisReplyParser(void* reply);

	~RedisReplyParser();

	// 发生在redis出错时
	bool IsEmpty() const;

	// 表示返回是空，发生在redis无值返回
	bool IsNil() const;

	bool GetError(std::string& value);

	// 跟string实现了互换
	void GetInteger(long long& value);

	void GetString(std::string& value);

	void GetString(char* value, unsigned int len);

	void GetOk(bool& value);

	template<typename T>
	void GetArray(T& values);

	template<typename T>
	void GetArray(std::set<T>& values);

	template<typename T1, typename T2>
	void GetMap(std::map<T1, T2>& values);

    template<typename T>
    void GetScan(long long& cursor, T& values);

protected:
	// 禁止拷贝
	RedisReplyParser(const RedisReplyParser&);

	RedisReplyParser& operator =(const RedisReplyParser&);

	bool GetValue(void* reply, std::string& value);

	template<typename T>
	bool GetValue(void* reply, T& value) {
		std::string val;
		if (GetValue(reply, val)) {
			std::istringstream iss(val);
			iss >> value;
			return true;
		}
		return false;
	}

private:
	void* _reply;
};

////////////////////////////////////////////////////////////////////

// 获取数组
template <typename T>
void RedisReplyParser::GetArray(T &values) {
	RedisException error;
	do {
		error = checkReply(_reply, REDIS_REPLY_ARRAY);
		if (!error.Empty()) {
			break;
		}
		
		iteratorReply(_reply, [&values, this](void* reply) {
			typename T::value_type v;
			DataConstruct(v);
			this->GetValue(reply, v);
			values.push_back(v);
		});
	} while (false);

	if (!error.Empty()) {
		throw error;
	}
}

template<typename T>
void RedisReplyParser::GetArray(std::set<T>& values) {
	RedisException error;
	do {
		error = checkReply(_reply, REDIS_REPLY_ARRAY);
		if (!error.Empty()) {
			break;
		}

		iteratorReply(_reply, [&values, this](void* reply) {
			typename T::value_type v;
			DataConstruct(v);
			this->GetValue(reply, v);
			values.insert(v);
		});
	} while (false);

	if (!error.Empty()) {
		throw error;
	}
}

template <typename T1, typename T2>
void RedisReplyParser::GetMap(std::map<T1, T2> &values) {
	RedisException error;
	do {
		error = checkReply(_reply, REDIS_REPLY_ARRAY);
		if (!error.Empty()) {
			break;
		}

		iteratorReply(_reply, [&values, this](void* ele1, void* ele2) {
			T1 value1;
			DataConstruct(value1);
			this->GetValue(ele1, value1);

			T2 value2;
			DataConstruct(value2);
			this->GetValue(ele2, value2);
		});
	} while (false);

	if (!error.Empty()) {
		throw error;
	}
}


template<typename T>
void RedisReplyParser::GetScan(long long& cursor, T& values) {
    RedisException error;
    do {
		error = checkReply(_reply, REDIS_REPLY_ARRAY);
		if (!error.Empty()) {
			break;
		}

		void* ele1 = 0;
		void* ele2 = 0;
		iteratorReply(_reply, &ele1, &ele2);

		if (!ele1 || !ele2) {
			error = RedisException(M_ERR_REDIS_SCAN);
			break;
		}

        // cursor
		this->GetValue(ele1, cursor);

		// data
		iteratorReply(ele2, [&values, this](void* ele) {
			typename T::value_type v;
			DataConstruct(v);
			this->GetValue(ele, v);
			values.push_back(v);
		});
    } while (false);

    if (!error.Empty()) {
		throw error;
	}
}

typedef std::shared_ptr<RedisReplyParser> RedisReplyParserPtr;


} // redis
} // async