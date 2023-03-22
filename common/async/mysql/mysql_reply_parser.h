/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include <memory>

namespace async {
namespace mysql {

struct MysqlReplyParser {
    MysqlReplyParser(const void* res, int err);

    ~MysqlReplyParser();

    // 获取受影响的行数
    int GetAffectedRow();

    // 获取结果集行数
    int GetNumRow();

    // 获取列数
    int GetField();

    const void* Next();

    int GetError();

    bool IsTimeout();

    const char* GetRowValue(const void* row, int idx);

    const void* _res;
    int _errno;
    bool _timeout;
    int _affect;
};

typedef std::shared_ptr<MysqlReplyParser> MysqlReplyParserPtr;

}
}