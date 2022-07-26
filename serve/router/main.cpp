/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#include "serve/serve/serve.h"

int main(int argc, char** argv) {
    Serve srv;
    srv.useRouter(true);
    srv.useAsync(false);
    if (!srv.init(argc, argv)) {
        return -1;
    }

    srv.start();
    return 0;
}