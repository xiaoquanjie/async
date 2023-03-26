/*----------------------------------------------------------------
// Copyright 2023
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_CURL

#include "common/async/curl/data.h"
#include <curl/curl.h>

namespace async {
namespace curl {

CurlReqData::~CurlReqData() {
    if (header) {
        curl_slist_free_all(header);
    }
}

 CurlCore::~CurlCore() {
    if (curlm) {
        curl_multi_cleanup(curlm);
    }
 }

}
}

#endif