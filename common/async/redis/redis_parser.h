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

	void GetValue(void* reply, std::string& value);

	template<typename T>
	void GetValue(void* reply, T& value);

private:
	void* _reply;
};

typedef std::shared_ptr<RedisReplyParser> RedisReplyParserPtr;


} // redis
} // async