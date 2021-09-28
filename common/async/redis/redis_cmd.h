/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include <vector>
#include <string>
#include <map>

namespace async {

namespace redis {

// redis »ù´¡commandÀà
struct BaseRedisCmd {
	std::vector<std::string> cmd;

	BaseRedisCmd();

	BaseRedisCmd(const std::vector<std::string>& v);

	std::string GetCmd() const;
};

/////////////////////////////////////////////////////////////////////////////////

struct SelectCmd : public BaseRedisCmd {
	SelectCmd(int db);
};

struct ExpireRedisCmd : public BaseRedisCmd {
	ExpireRedisCmd(const std::string& key, time_t expire);
};

struct DelRedisCmd : public BaseRedisCmd {
	DelRedisCmd(const std::initializer_list<std::string>& l);

	DelRedisCmd(const std::vector<std::string>& l);
};

struct ExistsRedisCmd : public BaseRedisCmd {
	ExistsRedisCmd(const std::string& key);
};

struct SetRedisCmd : public BaseRedisCmd {
	SetRedisCmd(const std::string& key, const std::string& value);
};

struct SetNxRedisCmd : public BaseRedisCmd {
	SetNxRedisCmd(const std::string& key, const std::string& value);

	SetNxRedisCmd(const std::string& key, const std::string& value, time_t expire);
};

struct SetExRedisCmd : public BaseRedisCmd {
	SetExRedisCmd(const std::string& key, const std::string& value, time_t expire);
};

struct GetRedisCmd : public BaseRedisCmd {
	GetRedisCmd(const std::string& key);
};

struct MgetRedisCmd : public BaseRedisCmd {
	MgetRedisCmd(std::initializer_list<std::string> l);

	MgetRedisCmd(const std::vector<std::string>& l);
};

struct IncrbyRedisCmd : public BaseRedisCmd {
	IncrbyRedisCmd(const std::string& key, int step);
};

struct DecrbyRedisCmd : public BaseRedisCmd {
	DecrbyRedisCmd(const std::string& key, int step);
};

struct StrlenRedisCmd : public BaseRedisCmd {
	StrlenRedisCmd(const std::string& key);
};

struct AppendRedisCmd : public BaseRedisCmd {
	AppendRedisCmd(const std::string& key, const std::string& value);
};

struct ZaddRedisCmd : public BaseRedisCmd {
	ZaddRedisCmd(const std::string& key, long long score, const std::string& member);
};

struct ZRangeRedisCmd : public BaseRedisCmd {
	ZRangeRedisCmd(const std::string& key, int beg_idx, int end_idx, bool withscores);
};

struct ZRevrangeRedisCmd : public BaseRedisCmd {
	ZRevrangeRedisCmd(const std::string& key, 
		int beg_idx, 
		int end_idx, 
		bool withscores);
};

struct ZRangeByScoreRedisCmd : public BaseRedisCmd {
	ZRangeByScoreRedisCmd(const std::string& key, 
		unsigned long long min, 
		unsigned long long max, 
		bool withscores);

	ZRangeByScoreRedisCmd(const std::string& key, 
		const std::string& min, 
		const std::string& max, 
		bool withscores);

	ZRangeByScoreRedisCmd(const std::string& key, 
		unsigned long long min, 
		unsigned long long max, 
		bool withscores,
		unsigned int count);

	ZRangeByScoreRedisCmd(const std::string& key, 
		const std::string& min, 
		const std::string& max, 
		bool withscores,
		unsigned int count);
};

struct ZrevrangeByScoreRedisCmd : public BaseRedisCmd {
	ZrevrangeByScoreRedisCmd(const std::string& key, 
		unsigned long long min, 
		unsigned long long max, 
		bool withscores);

	ZrevrangeByScoreRedisCmd(const std::string& key, 
		const std::string& min, 
		const std::string& max, 
		bool withscores);

	ZrevrangeByScoreRedisCmd(const std::string& key, 
		unsigned long long min, 
		unsigned long long max, 
		bool withscores,
		unsigned int count);

	ZrevrangeByScoreRedisCmd(const std::string& key, 
		const std::string& min, 
		const std::string& max, 
		bool withscores,
		unsigned int count);
};

struct ZincrbyRedisCmd : public BaseRedisCmd {
	ZincrbyRedisCmd(const std::string& key, unsigned long long score, const std::string& member);
};

struct ZscoreRedisCmd : public BaseRedisCmd {
	ZscoreRedisCmd(const std::string& key, const std::string& member);
};

struct ZrankRedisCmd : public BaseRedisCmd {
	ZrankRedisCmd(const std::string& key, const std::string& member);
};

struct ZrevrankRedisCmd : public BaseRedisCmd {
	ZrevrankRedisCmd(const std::string& key, const std::string& member);
};

struct ZcountRedisCmd : public BaseRedisCmd {
	ZcountRedisCmd(const std::string& key, const std::string& min, const std::string& max);
};

struct ZremRedisCmd : public BaseRedisCmd {
	ZremRedisCmd(const std::string& key, const std::string& member);
};

struct ZremrangebyscoreRedisCmd : public BaseRedisCmd {
	ZremrangebyscoreRedisCmd(const std::string& key, long long min_score, long long max_score);
};

struct ZremrangebyrankRedisCmd : public BaseRedisCmd {
	ZremrangebyrankRedisCmd(const std::string& key, long long start, long long stop);
};

struct ZcardRedisCmd : public BaseRedisCmd {
	ZcardRedisCmd(const std::string& key);
};

struct SetbitRedisCmd : public BaseRedisCmd {
	SetbitRedisCmd(const std::string& key, unsigned int offset, int value);
};

struct GetbitRedisCmd : public BaseRedisCmd {
	GetbitRedisCmd(const std::string& key, unsigned int offset);
};

struct LPushRedisCmd : public BaseRedisCmd {
	LPushRedisCmd(const std::string& key, const std::string& value);
};
struct RPushRedisCmd : public BaseRedisCmd {
	RPushRedisCmd(const std::string& key, const std::string& value);
};

struct LPopRedisCmd : public BaseRedisCmd {
	LPopRedisCmd(const std::string& key);
};

struct LIndexRedisCmd : public BaseRedisCmd {
	LIndexRedisCmd(const std::string& key, int index);
};

struct LRangeRedisCmd : public BaseRedisCmd {
	LRangeRedisCmd(const std::string& key, int beg_idx, int end_idx);
};

struct LTrimRedisCmd : public BaseRedisCmd {
	LTrimRedisCmd(const std::string& key, int beg_idx, int end_idx);
};

struct SaddRedisCmd : public BaseRedisCmd {
	SaddRedisCmd(const std::string& key, const std::initializer_list<std::string>& member);

	template<typename T>
	SaddRedisCmd(const std::string& key, const T& member) {
		cmd.push_back("SADD");
		cmd.push_back(key);
		for (auto iter = member.begin(); iter != member.end(); ++iter) {
			cmd.push_back(*iter);
		}
	}
};

struct SmembersRedisCmd : public BaseRedisCmd {
	SmembersRedisCmd(const std::string& key);
};

struct SRemRedisCmd : public BaseRedisCmd {
	SRemRedisCmd(const std::string& key, const std::string& member);
};

struct SScanRedisCmd : public BaseRedisCmd {
	SScanRedisCmd(const std::string& key, int64_t cursor, int64_t count);
};


struct HdelRedisCmd : public BaseRedisCmd {
	HdelRedisCmd(const std::string& key, const std::initializer_list<std::string>& members);
};

struct HgetRedisCmd : public BaseRedisCmd {
	HgetRedisCmd(const std::string& key, const std::string& member);
};

struct HsetRedisCmd : public BaseRedisCmd {
	HsetRedisCmd(const std::string& key, const std::string& member, const std::string& value);
};

struct HgetallRedisCmd : public BaseRedisCmd {
	HgetallRedisCmd(const std::string& key);
};

struct HexistsRedisCmd : public BaseRedisCmd {
	HexistsRedisCmd(const std::string& key, const std::string& member);
};

struct HincrbyRedisCmd : public BaseRedisCmd {
	HincrbyRedisCmd(const std::string& key, const std::string& member, int incr);
};

} // redis
} // async

/////////////////////////////////////////////////////////////////////////////////
