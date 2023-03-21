/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_REDIS

#include "common/async/redis/redis_cmd.h"
#include <string.h>

namespace async {
namespace redis {

BaseRedisCmd::BaseRedisCmd() {
}

BaseRedisCmd::BaseRedisCmd(const std::vector<std::string>& v) {
	cmd = v;
}

std::string BaseRedisCmd::GetCmd() const {
	std::string content;
	for (size_t idx = 0; idx < cmd.size(); ++idx) {
		if (cmd[idx].empty()) {
			content += "\'\' ";
		}
		else {
			content += cmd[idx] + " ";
		}
	}

	if (content.size()) {
		return content;
	}
	else {
		return "Emtpy Cmd";
	}
}

/////////////////////////////////////////////////////////////////////////////////

AuthCmd::AuthCmd(const std::string& pwd) {
	cmd.push_back("AUTH");
	cmd.push_back(pwd);
}

SelectCmd::SelectCmd(int db) {
	cmd.push_back("SELECT");
	cmd.push_back(std::to_string(db));
}

ExpireRedisCmd::ExpireRedisCmd(const std::string& key, time_t expire) {
	cmd.push_back("EXPIRE");
	cmd.push_back(key);
	cmd.push_back(std::to_string(expire));
}

DelRedisCmd::DelRedisCmd(const std::initializer_list<std::string>& l) {
	cmd.push_back("DEL");
	for (auto iter = l.begin(); iter != l.end(); ++iter) {
		cmd.push_back(*iter);
	}
}

DelRedisCmd::DelRedisCmd(const std::vector<std::string>& l) {
	cmd.push_back("DEL");
	for (auto iter = l.begin(); iter != l.end(); ++iter) {
		cmd.push_back(*iter);
	}
}

ExistsRedisCmd::ExistsRedisCmd(const std::string& key) {
	cmd.push_back("EXISTS");
	cmd.push_back(key);
}

SetRedisCmd::SetRedisCmd(const std::string& key, const std::string& value) {
	cmd.push_back("SET");
	cmd.push_back(key);
	cmd.push_back(value);
}

SetNxRedisCmd::SetNxRedisCmd(const std::string& key, const std::string& value) {
	cmd.push_back("SETNX");
	cmd.push_back(key);
	cmd.push_back(value);
}

SetNxRedisCmd::SetNxRedisCmd(const std::string& key, const std::string& value, time_t expire) {
	cmd.push_back("SET");
	cmd.push_back(key);
	cmd.push_back(value);
	cmd.push_back("EX");
	cmd.push_back(std::to_string(expire));
	cmd.push_back("NX");
}

SetExRedisCmd::SetExRedisCmd(const std::string& key, const std::string& value, time_t expire) {
	cmd.push_back("SETEX");
	cmd.push_back(key);
	cmd.push_back(std::to_string(expire));
	cmd.push_back(value);
}

GetRedisCmd::GetRedisCmd(const std::string& key) {
	cmd.push_back("GET");
	cmd.push_back(key);
}

MgetRedisCmd::MgetRedisCmd(std::initializer_list<std::string> l) {
	cmd.push_back("MGET");
	for (auto iter = l.begin(); iter != l.end(); ++iter) {
		cmd.push_back(*iter);
	}
}

MgetRedisCmd::MgetRedisCmd(const std::vector<std::string>& l) {
	cmd.push_back("MGET");
	for (auto iter = l.begin(); iter != l.end(); ++iter) {
		cmd.push_back(*iter);
	}
}

IncrbyRedisCmd::IncrbyRedisCmd(const std::string& key, int step) {
	cmd.push_back("INCRBY");
	cmd.push_back(key);
	cmd.push_back(std::to_string(step));
}

DecrbyRedisCmd::DecrbyRedisCmd(const std::string& key, int step) {
	cmd.push_back("DECRBY");
	cmd.push_back(key);
	cmd.push_back(std::to_string(step));
}

StrlenRedisCmd::StrlenRedisCmd(const std::string& key) {
	cmd.push_back("STRLEN");
	cmd.push_back(key);
}

AppendRedisCmd::AppendRedisCmd(const std::string& key, const std::string& value) {
	cmd.push_back("APPEND");
	cmd.push_back(key);
	cmd.push_back(value);
}

ZaddRedisCmd::ZaddRedisCmd(const std::string& key, long long score, const std::string& member) {
	cmd.push_back("ZADD");
	cmd.push_back(key);
	cmd.push_back(std::to_string(score));
	cmd.push_back(member);
}

ZRangeRedisCmd::ZRangeRedisCmd(const std::string& key, int beg_idx, int end_idx, bool withscores) {
	cmd.push_back("ZRANGE");
	cmd.push_back(key);
	cmd.push_back(std::to_string(beg_idx));
	cmd.push_back(std::to_string(end_idx));
	if (withscores) {
		cmd.push_back("WITHSCORES");
	}
}

ZRevrangeRedisCmd::ZRevrangeRedisCmd(const std::string& key, int beg_idx, int end_idx, bool withscores) {
	cmd.push_back("ZREVRANGE");
	cmd.push_back(key);
	cmd.push_back(std::to_string(beg_idx));
	cmd.push_back(std::to_string(end_idx));
	if (withscores) {
		cmd.push_back("WITHSCORES");
	}
}

ZRangeByScoreRedisCmd::ZRangeByScoreRedisCmd(const std::string& key, unsigned long long min, unsigned long long max, bool withscores) {
	cmd.push_back("ZRANGEBYSCORE");
	cmd.push_back(key);
	cmd.push_back(std::to_string(min));
	cmd.push_back(std::to_string(max));
	if (withscores) {
		cmd.push_back("WITHSCORES");
	}
}

ZRangeByScoreRedisCmd::ZRangeByScoreRedisCmd(const std::string& key, 
	const std::string& min, 
	const std::string& max, 
	bool withscores) {
	cmd.push_back("ZRANGEBYSCORE");
	cmd.push_back(key);
	cmd.push_back(min);
	cmd.push_back(max);
	if (withscores) {
		cmd.push_back("WITHSCORES");
	}
}

ZRangeByScoreRedisCmd::ZRangeByScoreRedisCmd(const std::string& key, 
	unsigned long long min, 
	unsigned long long max, 
	bool withscores,
	unsigned int count) {
	cmd.push_back("ZRANGEBYSCORE");
	cmd.push_back(key);
	cmd.push_back(std::to_string(min));
	cmd.push_back(std::to_string(max));
	if (withscores) {
		cmd.push_back("WITHSCORES");
	}

	cmd.push_back("LIMIT");
    cmd.push_back("0");
    cmd.push_back(std::to_string(count));
}

ZRangeByScoreRedisCmd::ZRangeByScoreRedisCmd(const std::string& key, 
	const std::string& min, 
	const std::string& max, 
	bool withscores,
	unsigned int count) {
	cmd.push_back("ZRANGEBYSCORE");
	cmd.push_back(key);
	cmd.push_back(min);
	cmd.push_back(max);
	if (withscores) {
		cmd.push_back("WITHSCORES");
	}

	cmd.push_back("LIMIT");
    cmd.push_back("0");
    cmd.push_back(std::to_string(count));
}

ZrevrangeByScoreRedisCmd::ZrevrangeByScoreRedisCmd(const std::string& key, 
	unsigned long long min, 
	unsigned long long max, 
	bool withscores) {
	cmd.push_back("ZREVRANGEBYSCORE");
	cmd.push_back(key);
	cmd.push_back(std::to_string(min));
	cmd.push_back(std::to_string(max));
	if (withscores) {
		cmd.push_back("WITHSCORES");
	}
}

ZrevrangeByScoreRedisCmd::ZrevrangeByScoreRedisCmd(const std::string& key, 
	const std::string& min, 
	const std::string& max, 
	bool withscores) {
	cmd.push_back("ZREVRANGEBYSCORE");
	cmd.push_back(key);
	cmd.push_back((min));
	cmd.push_back((max));
	if (withscores) {
		cmd.push_back("WITHSCORES");
	}
}

ZrevrangeByScoreRedisCmd::ZrevrangeByScoreRedisCmd(const std::string& key, 
	unsigned long long min, 
	unsigned long long max, 
	bool withscores,
	unsigned int count) {
	cmd.push_back("ZREVRANGEBYSCORE");
	cmd.push_back(key);
	cmd.push_back(std::to_string(min));
	cmd.push_back(std::to_string(max));
	if (withscores) {
		cmd.push_back("WITHSCORES");
	}

	cmd.push_back("LIMIT");
    cmd.push_back("0");
    cmd.push_back(std::to_string(count));
}

ZrevrangeByScoreRedisCmd::ZrevrangeByScoreRedisCmd(const std::string& key, 
	const std::string& min, 
	const std::string& max, 
	bool withscores,
	unsigned int count) {
	cmd.push_back("ZREVRANGEBYSCORE");
	cmd.push_back(key);
	cmd.push_back((min));
	cmd.push_back((max));
	if (withscores) {
		cmd.push_back("WITHSCORES");
	}

	cmd.push_back("LIMIT");
    cmd.push_back("0");
    cmd.push_back(std::to_string(count));
}

ZincrbyRedisCmd::ZincrbyRedisCmd(const std::string& key, unsigned long long score, const std::string& member) {
	cmd.push_back("ZINCRBY");
	cmd.push_back(key);
	cmd.push_back(std::to_string(score));
	cmd.push_back(member);
}

ZscoreRedisCmd::ZscoreRedisCmd(const std::string& key, const std::string& member) {
	cmd.push_back("ZSCORE");
	cmd.push_back(key);
	cmd.push_back(member);
}

ZrankRedisCmd::ZrankRedisCmd(const std::string& key, const std::string& member) {
	cmd.push_back("ZRANK");
	cmd.push_back(key);
	cmd.push_back(member);	
}

ZrevrankRedisCmd::ZrevrankRedisCmd(const std::string& key, const std::string& member) {
	cmd.push_back("ZREVRANK");
	cmd.push_back(key);
	cmd.push_back(member);	
}

ZcountRedisCmd::ZcountRedisCmd(const std::string& key, const std::string& min, const std::string& max) {
	cmd.push_back("ZCOUNT");
	cmd.push_back(key);
	cmd.push_back(min);
	cmd.push_back(max);
}

ZremRedisCmd::ZremRedisCmd(const std::string& key, const std::string& member) {
	cmd.push_back("ZREM");
	cmd.push_back(key);
	cmd.push_back(member);
}

ZremrangebyscoreRedisCmd::ZremrangebyscoreRedisCmd(const std::string& key, long long min_score, long long max_score) {
	cmd.push_back("ZREMRANGEBYSCORE");
	cmd.push_back(key);
	cmd.push_back(std::to_string(min_score));
	cmd.push_back(std::to_string(max_score));
}

ZremrangebyrankRedisCmd::ZremrangebyrankRedisCmd(const std::string& key, long long start, long long stop) {
	cmd.push_back("ZREMRANGEBYRANK");
	cmd.push_back(key);
	cmd.push_back(std::to_string(start));
	cmd.push_back(std::to_string(stop));
}

ZcardRedisCmd::ZcardRedisCmd(const std::string& key) {
	cmd.push_back("ZCARD");
	cmd.push_back(key);
}

SetbitRedisCmd::SetbitRedisCmd(const std::string& key, unsigned int offset, int value) {
	cmd.push_back("SETBIT");
	cmd.push_back(key);
	cmd.push_back(std::to_string(offset));
	cmd.push_back(std::to_string(value));
}

GetbitRedisCmd::GetbitRedisCmd(const std::string& key, unsigned int offset) {
	cmd.push_back("GETBIT");
	cmd.push_back(key);
	cmd.push_back(std::to_string(offset));
}

LPushRedisCmd::LPushRedisCmd(const std::string& key, const std::string& value) {
	cmd.push_back("LPUSH");
	cmd.push_back(key);
	cmd.push_back(value);
}
RPushRedisCmd::RPushRedisCmd(const std::string& key, const std::string& value) {
	cmd.push_back("RPUSH");
	cmd.push_back(key);
	cmd.push_back(value);
}

LPopRedisCmd::LPopRedisCmd(const std::string& key) {
	cmd.push_back("LPOP");
	cmd.push_back(key);
}

LIndexRedisCmd::LIndexRedisCmd(const std::string& key, int index) {
	cmd.push_back("LINDEX");
	cmd.push_back(key);
	cmd.push_back(std::to_string(index));
}

LRangeRedisCmd::LRangeRedisCmd(const std::string& key, int beg_idx, int end_idx) {
	cmd.push_back("LRANGE");
	cmd.push_back(key);
	cmd.push_back(std::to_string(beg_idx));
	cmd.push_back(std::to_string(end_idx));
}

LTrimRedisCmd::LTrimRedisCmd(const std::string& key, int beg_idx, int end_idx) {
	cmd.push_back("LTRIM");
	cmd.push_back(key);
	cmd.push_back(std::to_string(beg_idx));
	cmd.push_back(std::to_string(end_idx));
}

SaddRedisCmd::SaddRedisCmd(const std::string& key, const std::initializer_list<std::string>& member) {
	cmd.push_back("SADD");
	cmd.push_back(key);
	for (auto iter = member.begin(); iter != member.end(); ++iter) {
		cmd.push_back(*iter);
	}
}

SmembersRedisCmd::SmembersRedisCmd(const std::string& key) {
	cmd.push_back("SMEMBERS");
	cmd.push_back(key);
}

SRemRedisCmd::SRemRedisCmd(const std::string& key, const std::string& member) {
	cmd.push_back("SREM");
	cmd.push_back(key);
	cmd.push_back(member);
}

SScanRedisCmd::SScanRedisCmd(const std::string& key, int64_t cursor, int64_t count) {
	cmd.push_back("SSCAN");
	cmd.push_back(key);
	cmd.push_back(std::to_string(cursor));
    cmd.push_back("COUNT");
    cmd.push_back(std::to_string(count));
}


HdelRedisCmd::HdelRedisCmd(const std::string& key, const std::initializer_list<std::string>& members) {
	cmd.push_back("HDEL");
	cmd.push_back(key);
	for (auto& item : members) {
		cmd.push_back(item);
	}
}

HgetRedisCmd::HgetRedisCmd(const std::string& key, const std::string& member) {
	cmd.push_back("HGET");
	cmd.push_back(key);
	cmd.push_back(member);
}

HsetRedisCmd::HsetRedisCmd(const std::string& key, const std::string& member, const std::string& value) {
	cmd.push_back("HSET");
	cmd.push_back(key);
	cmd.push_back(member);
	cmd.push_back(value);
}

HgetallRedisCmd::HgetallRedisCmd(const std::string& key) {
	cmd.push_back("HGETALL");
	cmd.push_back(key);
}

HexistsRedisCmd::HexistsRedisCmd(const std::string& key, const std::string& member) {
	cmd.push_back("HEXISTS");
	cmd.push_back(key);
	cmd.push_back(member);
}

HincrbyRedisCmd::HincrbyRedisCmd(const std::string& key, const std::string& member, int incr) {
	cmd.push_back("HINCRBY");
	cmd.push_back(key);
	cmd.push_back(member);
	cmd.push_back(std::to_string(incr));
}

HmsetRedisCmd::HmsetRedisCmd(const std::string& key, const std::map<std::string, std::string>& member_to_value) {
    cmd.push_back("HMSET");
    cmd.push_back(key);
    for(auto const& iter : member_to_value) {
        cmd.push_back(iter.first);
        cmd.push_back(iter.second);
    }
}

HmgetRedisCmd::HmgetRedisCmd(const std::string& key, const std::vector<std::string>& members) {
    cmd.push_back("HMGET");
    cmd.push_back(key);
    cmd.insert(cmd.end(), members.begin(), members.end());
}

} // redis
} // async

#endif

/////////////////////////////////////////////////////////////////////////////////
