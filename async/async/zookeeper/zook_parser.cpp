/*----------------------------------------------------------------
// Copyright 2023
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_ZOOKEEPER

#include "async/async/zookeeper/zook_parser.h"
#include <zookeeper/zookeeper.h>

namespace async {
namespace zookeeper {

// @rc的值可具体查看ZOO_ERRORS，rc是timeout的情况下s
ZookParser::ZookParser(int rc, const void* s, const char* v, int len) {
    this->rc = rc;
    if (rc == ZOK) {
        if (s) {
            this->stat = new struct Stat();
            *((struct Stat*)(this->stat)) = *((const struct Stat*)(s));
        } else {
            this->stat = 0;
        }
        if (v) {
            this->value = new std::string(v, len);
        } else {
            this->value = 0;
        }
    } else {
        this->stat = 0;
        this->value = 0;
    }
}

ZookParser::~ZookParser() {
    delete this->value;
    delete (struct Stat*)(this->stat);

    for (auto& acl : this->acls) {
        delete acl;
    }
    for (auto& child : this->childs) {
        delete child;
    }
}

const std::string& ZookParser::getValue() {
    if (this->value) {
        return *this->value;
    } else {
        static std::string empty;
        return empty;
    }
}

int ZookParser::getRc() {
    return this->rc;
}

const char* ZookParser::error() {
    return zerror(this->rc);
}

void ZookParser::setChilds(const void* c) {
    const struct String_vector* childs = (const struct String_vector*)c;
    for (int32_t i = 0; i < childs->count; i++) {
        char* child = childs->data[i];
        this->childs.push_back(new std::string(child));
    }
}

const std::vector<std::string*>& ZookParser::getChilds() {
    return this->childs;
}

void ZookParser::setAcls(const void* l) {
    const struct ACL_vector* acl = (const struct ACL_vector*)l;
    for (int32_t i = 0; i < acl->count; i++) {
        struct ACL& a = acl->data[i];

        ZookAcl* za = new ZookAcl;
        za->perms = a.perms;
        za->scheme = a.id.scheme;
        za->id = a.id.id;
        this->acls.push_back(za);
    }
}

const std::vector<ZookParser::ZookAcl*>& ZookParser::getAcls() {
    return this->acls;
}

bool ZookParser::allPerm(uint32_t perms) {
    return (perms & ZOO_PERM_ALL) == 1;
}

bool ZookParser::readPerm(uint32_t perms) {
    return (perms & ZOO_PERM_READ) == 1;
}

bool ZookParser::writePerm(uint32_t perms) {
    return (perms & ZOO_PERM_WRITE) == 1;
}

bool ZookParser::DeletePerm(uint32_t perms) {
    return (perms & ZOO_PERM_DELETE) == 1;
}

bool ZookParser::CreatePerm(uint32_t perms) {
    return (perms & ZOO_PERM_CREATE) == 1;
}   

bool ZookParser::AdminPerm(uint32_t perms) {
    return (perms & ZOO_PERM_ADMIN) == 1;
}

void* ZookParser::getStat() {
    return this->stat;
}

// 这是导致创建znode更改的事务ID
int64_t ZookParser::getStatCzxid() {
    return ((struct Stat*)(this->stat))->czxid;
}

// 这是最后修改znode更改的事务ID。
int64_t ZookParser::getStatMzxid() {
    return ((struct Stat*)(this->stat))->mzxid;
}

// 表示从1970-01-01T00:00:00Z开始以毫秒为单位的znode创建时间
int64_t ZookParser::getStatCtime() {
    return ((struct Stat*)(this->stat))->ctime;
}

// 表示从1970-01-01T00:00:00Z开始以毫秒为单位的znode最近修改时间
int64_t ZookParser::getStatMtime() {
    return ((struct Stat*)(this->stat))->mtime;
}

// 表示对该znode的数据所做的更改次数
int32_t ZookParser::getStatVersion() {
    return ((struct Stat*)(this->stat))->version;
}

// 这表示对此znode的子节点进行的更改次数
int32_t ZookParser::getStatCversion() {
    return ((struct Stat*)(this->stat))->cversion;
}

// 表示对此znode的ACL进行更改的次数
int32_t ZookParser::getStatAversion() {
    return ((struct Stat*)(this->stat))->aversion;
}

// 如果znode是ephemeral类型节点，则这是znode所有者的 session ID。 如果znode不是ephemeral节点，则该字段设置为零。
int64_t ZookParser::getStatEphemeralOwner() {
    return ((struct Stat*)(this->stat))->ephemeralOwner;
}

// 这是znode数据字段的长度
int32_t ZookParser::getStatDataLength() {
    return ((struct Stat*)(this->stat))->dataLength;
}

// 这表示znode的子节点的数量
int32_t ZookParser::getStatNumChildren() {
    return ((struct Stat*)(this->stat))->numChildren;
}

// 这是用于添加或删除子节点的znode更改的事务ID
int64_t ZookParser::getStatPzxid() {
    return ((struct Stat*)(this->stat))->pzxid;
}

bool ZookParser::zTimeout() {
    return this->rc == ZOPERATIONTIMEOUT;
}

bool ZookParser::zOk() {
    return this->rc == ZOK;
}

bool ZookParser::zNoNode() {
    return this->rc == ZNONODE;
}

bool ZookParser::zNoAuth() {
    return this->rc == ZNOAUTH;
}

// invalid ACL specified
bool ZookParser::zInvalIdAcl() {
    return this->rc == ZINVALIDACL;
}

// expected version does not match actual version.
bool ZookParser::zBadVersion() {
    return this->rc == ZBADVERSION;
}

// authentication failed
bool ZookParser::zAuthFailed() {
    return this->rc == ZAUTHFAILED;
}

bool ZookParser::zNotEmpty() {
    return this->rc == ZNOTEMPTY;
}

}
}

#endif