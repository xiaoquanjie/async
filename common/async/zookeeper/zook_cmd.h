/*----------------------------------------------------------------
// Copyright 2023
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include <string>
#include <vector>

namespace async {
namespace zookeeper {

enum cmd_type {
    get_cmd = 1,                // 获取节点
    create_cmd = 2,             // 创建节点
    delete_cmd = 3,             // 删除节点
    exists_cmd = 4,             // 存在节点
    set_cmd = 5,                // 设置节点
    get_acl_cmd = 6,            // 获取权限
    set_acl_cmd = 7,      
    list_cmd = 8,               // 列出子节点      
};

struct BaseZookCmd {
    uint32_t cmdType = 0;
    virtual ~BaseZookCmd() {}
};

struct GetCmd : public BaseZookCmd {
    GetCmd();

    GetCmd(const std::string& path);

    std::string path;           // 节点路径
};

// warn: std::make_shared不支持std::initializer_list
struct CreateCmd : public BaseZookCmd {
    CreateCmd();

    CreateCmd(const std::string& path, 
        const std::string& value, 
        const std::initializer_list<std::string>& acls = {});

    CreateCmd(const std::string& path, 
        const std::string& value,
        bool ephemeral,
        bool withSeq,
        int64_t ttl,
        const std::initializer_list<std::string>& acls = {});

    std::string path;               // 节点路径
    std::string value;              // 节点值
    bool ephemeral = false;         // 临时的, 临时节点不能带ttl
    bool withSeq = false;           // 带有序号
    int64_t ttl = 0;                // ttl，毫秒
    std::vector<std::string> acls;  // 格式是: scheme:id:perm，例如：world:anyone:cdrwa, ip:xx.xx.xx.xx:cdrwa, auth::cdrwa, digest:user:pwd:cdrwa
};

struct DeleteCmd : public BaseZookCmd {
    DeleteCmd();

    DeleteCmd(const std::string& path, int32_t version = -1);

    std::string path;               // 节点路径
    int32_t version = -1;           // 版本号, -1不检查版本号
};

struct ExistsCmd : public BaseZookCmd {
    ExistsCmd();

    ExistsCmd(const std::string& path);

    std::string path;               // 节点路径
};

struct SetCmd : public BaseZookCmd {
    SetCmd();

    SetCmd(const std::string& path, const std::string& value, int32_t version = -1);

    std::string path;               // 节点路径
    std::string value;              // 节点值
    int32_t version = -1;           // 版本号, -1不检查版本号
};

struct GetAclCmd : public BaseZookCmd {
    GetAclCmd();

    GetAclCmd(const std::string& path);

    std::string path;               // 节点路径
};

struct SetAclCmd : public BaseZookCmd {
    SetAclCmd();

    SetAclCmd(const std::string& path,  
        const std::initializer_list<std::string>& acls,
        int32_t version = -1);

    std::string path;               // 节点路径
    int32_t version = -1;           // 版本号, -1不检查版本号
    std::vector<std::string> acls;  // 格式是: scheme:id:perm，例如：world:anyone:cdrwa, ip:xx.xx.xx.xx:cdrwa, auth::cdrwa, digest:user:pwd:cdrwa
};

struct ListCmd : public BaseZookCmd {
    ListCmd();

    ListCmd(const std::string& path);

    std::string path;               // 节点路径
};


}
}