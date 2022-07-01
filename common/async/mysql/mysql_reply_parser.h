/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include <memory>

namespace async {
namespace mysql {

struct MysqlReplyParser {
    MysqlReplyParser(const void* res, int affect, int err);

    ~MysqlReplyParser();

    // 获取受影响的行数
    int GetAffectedRow();

    // 获取列数
    int GetField();

    const void* Next();

    int GetError();

    const void* _res;
    int _errno;
    int _affectedRow;
};

typedef std::shared_ptr<MysqlReplyParser> MysqlReplyParserPtr;

}
}