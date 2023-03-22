/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_MONGO

#include "common/co_async/mongo/co_mongo.h"
#include "common/co_async/promise.h"
#include "common/log.h"

namespace co_async {
namespace mongo {

////////////////////////////////////////////////////////////////////

std::pair<int, async::mongo::MongoReplyParserPtr> execute(const std::string &uri, const async::mongo::BaseMongoCmd &cmd, const TimeOut& t) {
    auto res = co_async::promise([&uri, &cmd](co_async::Resolve resolve, co_async::Reject reject) {
        async::mongo::execute(uri, cmd, [resolve](async::mongo::MongoReplyParserPtr parser) {
            resolve(parser);
        });
    }, t());

    std::pair<int, async::mongo::MongoReplyParserPtr> ret = std::make_pair(res.first, nullptr);
    if (co_async::checkOk(res)) {
        ret.second = co_async::getOk<async::mongo::MongoReplyParser>(res);
        auto parser = ret.second;
        if (!parser->IsOk()) {
            log("[co_async_mongo] failed to execute mongo|%s|%s", cmd.DebugString().c_str(), parser->What());
        }
    }
    else {
        log("[co_async_mongo] [timeout] execute mongo|%s", cmd.DebugString().c_str());
    }

    return ret;
}

std::pair<int, bool> execute(const std::string &uri, const async::mongo::BaseMongoCmd &cmd, std::string* jsonResult, const TimeOut& t) {
    auto res = execute(uri, cmd, t);
    std::pair<int, bool> ret = std::make_pair(res.first, false);

    if (co_async::checkOk(res)) {
        auto parser = res.second;
        ret.second = parser->IsOk();

        if (jsonResult) {
            jsonResult->clear();
            jsonResult->append("[");
            int idx = 0;
            while (true) {
                char *json = parser->NextJson();
                if (!json) {
                    break;
                }
                if (idx != 0) {
                    jsonResult->append(",");
                }
                ++idx;
                jsonResult->append(json);
                parser->FreeJson(json);
            }
            jsonResult->append("]");
        }
    }

    return ret;
}

std::pair<int, bool> execute(const std::string &uri, const async::mongo::BaseMongoCmd &cmd, int* deletedCount, const TimeOut& t) {
    auto res = execute(uri, cmd, t);
    std::pair<int, bool> ret = std::make_pair(res.first, false);

    if (co_async::checkOk(res)) {
        auto parser = res.second;
        ret.second = parser->IsOk();

        if (deletedCount) {
            parser->GetDeletedCount(*deletedCount);
        }
    }

    return ret;
}

std::pair<int, bool> execute(const std::string &uri, const async::mongo::BaseMongoCmd &cmd, int* modifiedCount, int* matchedCount, int* upsertedCount, const TimeOut& t) {
    auto res = execute(uri, cmd, t);
    std::pair<int, bool> ret = std::make_pair(res.first, false);

    if (co_async::checkOk(res)) {
        auto parser = res.second;
        ret.second = parser->IsOk();

        if (modifiedCount) {
            parser->GetModifiedCount(*modifiedCount);
        }
        if (matchedCount) {
            parser->GetMatchedCount(*matchedCount);
        }
        if (upsertedCount) {
            parser->GetUpsertedCount(*upsertedCount);
        }
    }

    return ret;
}

}
}

#endif