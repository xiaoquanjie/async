/*----------------------------------------------------------------
// Copyright 2023
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include <memory>
#include <string>

namespace async {
namespace curl {

struct CurlParser {
    CurlParser(const std::string& value, int32_t curlCode, int32_t rspCode);

    void swapValue(std::string& value);

    std::string& getValue();

    int32_t getCurlCode();

    int32_t getRspCode();

    // 库操作成功
    bool curlOk();
private:
    std::string value;
    // curl操作的错误码
    int32_t curlCode = 0;
    // http协议的错误码, 为0则表示服务器没有返回，一般是200是成功码
    int32_t rspCode = 0;
};

typedef std::shared_ptr<CurlParser> CurlParserPtr;

}
}