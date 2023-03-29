/*----------------------------------------------------------------
// Copyright 2023
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include <memory>
#include <functional>
#include <string>
#include <vector>

namespace async {
namespace zookeeper {

// zookeeper结果解析
struct ZookParser {
    struct ZookAcl {
        std::string scheme;
        std::string id;
        int32_t perms = 0;
    };

    ZookParser(int rc, const void* s, const char* v, int len);

    ~ZookParser();

    const std::string& getValue();

    int getRc();

    const char* error();

    void setChilds(const void* c);

    const std::vector<std::string*>& getChilds();

    // 设置权限
    void setAcls(const void* acl);

    const std::vector<ZookAcl*>& getAcls();

    // 全部权限
    bool allPerm(uint32_t perms);

    bool readPerm(uint32_t perms);

    bool writePerm(uint32_t perms);

    bool DeletePerm(uint32_t perms);

    bool CreatePerm(uint32_t perms);

    bool AdminPerm(uint32_t perms);

    void* getStat();

    // 这是导致创建znode更改的事务ID
    int64_t getStatCzxid();

    // 这是最后修改znode更改的事务ID。
    int64_t getStatMzxid();

    // 表示从1970-01-01T00:00:00Z开始以毫秒为单位的znode创建时间
    int64_t getStatCtime();

    // 表示从1970-01-01T00:00:00Z开始以毫秒为单位的znode最近修改时间
    int64_t getStatMtime();

    // 表示对该znode的数据所做的更改次数
    int32_t getStatVersion();

    // 这表示对此znode的子节点进行的更改次数
    int32_t getStatCversion();

    // 表示对此znode的ACL进行更改的次数
    int32_t getStatAversion();

    // 如果znode是ephemeral类型节点，则这是znode所有者的 session ID。 如果znode不是ephemeral节点，则该字段设置为零。
    int64_t getStatEphemeralOwner();

    // 这是znode数据字段的长度
    int32_t getStatDataLength();

    // 这表示znode的子节点的数量
    int32_t getStatNumChildren();

    // 这是用于添加或删除子节点的znode更改的事务ID
    int64_t getStatPzxid();

    bool zTimeout();

    bool zOk();

    bool zNoNode();

    bool zNoAuth();

    // invalid ACL specified
    bool zInvalIdAcl();

    // expected version does not match actual version.
    bool zBadVersion();

    // authentication failed
    bool zAuthFailed();

    bool zNotEmpty();
private:
    int rc = 0;
    void* stat = 0;
    std::string* value;
    std::vector<ZookAcl*> acls;
    std::vector<std::string*> childs;
};

typedef std::shared_ptr<ZookParser> ZookParserPtr;

}
}