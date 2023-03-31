/*----------------------------------------------------------------
// Copyright 2023
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_ZOOKEEPER

#include "async/async/zookeeper/zook_cmd.h"

namespace async {
namespace zookeeper {

GetCmd::GetCmd() {
    this->cmdType = get_cmd;
}

GetCmd::GetCmd(const std::string& path) {
    this->cmdType = get_cmd;
    this->path = path;
}

CreateCmd::CreateCmd() {
    this->cmdType = create_cmd;
}

CreateCmd::CreateCmd(const std::string& path, const std::string& value, const std::initializer_list<std::string>& acls) {
    this->cmdType = create_cmd;
    this->path = path;
    this->value = value;
    for (auto s : acls) {
        this->acls.push_back(s);
    }
}

CreateCmd::CreateCmd(const std::string& path, 
    const std::string& value,
    bool ephemeral,
    bool withSeq,
    int64_t ttl,
    const std::initializer_list<std::string>& acls) 
{
    this->cmdType = create_cmd;
    this->path = path;
    this->value = value;
    this->ephemeral = ephemeral;
    this->withSeq = withSeq;
    this->ttl = ttl;
    for (auto s : acls) {
        this->acls.push_back(s);
    }
}

DeleteCmd::DeleteCmd() {
    this->cmdType = delete_cmd;
}

DeleteCmd::DeleteCmd(const std::string& path, int32_t version) {
    this->cmdType = delete_cmd;
    this->path = path;
    this->version = version;
}

ExistsCmd::ExistsCmd() {
    this->cmdType = exists_cmd;
}

ExistsCmd::ExistsCmd(const std::string& path) {
    this->cmdType = exists_cmd;
    this->path = path;
}

SetCmd::SetCmd() {
    this->cmdType = set_cmd;
}

SetCmd::SetCmd(const std::string& path, const std::string& value, int32_t version) {
    this->cmdType = set_cmd;
    this->path = path;
    this->value = value;
    this->version = version;
}

GetAclCmd::GetAclCmd() {
    this->cmdType = get_acl_cmd;
}

GetAclCmd::GetAclCmd(const std::string& path) {
    this->cmdType = get_acl_cmd;
    this->path = path;
}

SetAclCmd::SetAclCmd() {
    this->cmdType = set_acl_cmd;
}

SetAclCmd::SetAclCmd(const std::string& path,  
    const std::initializer_list<std::string>& acls,
    int32_t version) {
    this->cmdType = set_acl_cmd;
    this->path = path;
    for (auto s : acls) {
        this->acls.push_back(s);
    }
    this->version = version;
}

ListCmd::ListCmd() {
    this->cmdType = list_cmd;
}


ListCmd::ListCmd(const std::string& path) {
    this->cmdType = list_cmd;
    this->path = path;
}

}
}

#endif