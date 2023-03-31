/*----------------------------------------------------------------
// Copyright 2023
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_CURL

#include "async/async/curl/curl_parser.h"
#include <curl/curl.h>

namespace async {
namespace curl {

CurlParser::CurlParser(const std::string& value, int32_t curlCode, int32_t rspCode) {
    this->value = value;
    this->curlCode = curlCode;
    this->rspCode = rspCode;
}

void CurlParser::swapValue(std::string& value) {
    this->value.swap(value);
}

std::string& CurlParser::getValue() {
    return this->value;
}

int32_t CurlParser::getCurlCode() {
    return this->curlCode;
}

int32_t CurlParser::getRspCode() {
    return this->rspCode;
}

bool CurlParser::curlOk() {
    return this->curlCode == CURLE_OK;
}

}
}

#endif