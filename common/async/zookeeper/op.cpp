/*----------------------------------------------------------------
// Copyright 2023
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_ASYNC_ZOOKEEPER

#include "common/async/zookeeper/data.h"
#include "common/async/zookeeper/callback.h"
#include "common/async/zookeeper/op.h"
#include "common/async/zookeeper/zook_parser.h"
#include <cassert>

namespace async {

void split(const std::string source, const std::string &separator, std::vector<std::string> &array);

namespace zookeeper {

// acl解析
void aclParse(const std::vector<std::string>& acls, struct ACL_vector& acl, std::vector<ZookParser::ZookAcl>& zacl) {
    if (acls.empty()) {
        acl = ZOO_OPEN_ACL_UNSAFE;
        return;
    }

    for (auto s : acls) {
        std::vector<std::string> v;
        split(s, ":", v);

        ZookParser::ZookAcl z;
        z.scheme = v[0];
        std::string perms;

        if (v[0] == "world") {
            z.id = v[1];
            perms = v[2];
        }
        else if (v[0] == "ip") {
            z.id = v[1];
            perms = v[2];
        }
        else if (v[0] == "auth") {
            z.id = "";
            perms = v[2];
        }
        else if (v[0] == "digest") {
            z.id = v[1] + ":" + v[2];
            perms = v[3];
        }

        for (auto c : perms) {
            if (c == 'r') {
                z.perms |= ZOO_PERM_READ;
            }
            else if (c == 'w') {
                z.perms |= ZOO_PERM_WRITE;
            }
            else if (c == 'c') {
                z.perms |= ZOO_PERM_CREATE;
            }
            else if (c == 'd') {
                z.perms |= ZOO_PERM_DELETE;
            }
            else if (c == 'a') {
                z.perms |= ZOO_PERM_ADMIN;
            }
        }

        zacl.push_back(z);
    }

    int l = zacl.size();
    acl.count = l;
    acl.data = (struct ACL*)malloc(sizeof(struct ACL) * l);
    for (size_t i = 0; i < zacl.size(); i++) {
        acl.data[i].perms = zacl[i].perms;
        acl.data[i].id.scheme = const_cast<char*>(zacl[i].scheme.c_str());
        acl.data[i].id.id = const_cast<char*>(zacl[i].id.c_str());
    }
}

void opGetCmd(ZookCorePtr c, ZookReqDataPtr req) {
    auto cmd = std::static_pointer_cast<GetCmd>(req->cmd);
    InvokeData* invoke = new InvokeData();
    invoke->req = req;
    invoke->c = c;

    int rc = zoo_aget(c->conn->zh,
        cmd->path.c_str(),
        0, /*no watch*/
        onDataCb,
        invoke
    );

    if (!checkOk(rc, "zoo_aget")) {
        delete invoke;
        auto rsp = std::make_shared<ZookRspData>();
        rsp->req = req;
        rsp->parser = std::make_shared<ZookParser>(rc, nullptr, nullptr, 0);
        onPushRsp(rsp, req->tData);
    }
}

void onCreateCmd(ZookCorePtr c, ZookReqDataPtr req) {
    auto cmd = std::static_pointer_cast<CreateCmd>(req->cmd);
    InvokeData* invoke = new InvokeData();
    invoke->req = req;
    invoke->c = c;
    
    ACL_vector acl;
    std::vector<ZookParser::ZookAcl> zacl;
    aclParse(cmd->acls, acl, zacl);

    int mode = 0;
    if (cmd->ephemeral) {
        if (cmd->withSeq) {
            mode = ZOO_EPHEMERAL_SEQUENTIAL;
        } else {
            mode = ZOO_EPHEMERAL;
        }
    } else {
        if (cmd->withSeq) {
            if (cmd->ttl > 0) {
                mode = ZOO_PERSISTENT_SEQUENTIAL_WITH_TTL;
            } else {
                mode = ZOO_PERSISTENT_SEQUENTIAL;
            }
        } else {
            if (cmd->ttl > 0) {
                mode = ZOO_PERSISTENT_WITH_TTL;
            } else {
                mode = ZOO_PERSISTENT;
            }
        }
    }

    int rc = 0;
    if (!cmd->ephemeral && cmd->ttl > 0) {
        rc = zoo_acreate_ttl(
            c->conn->zh,
            cmd->path.c_str(),
            cmd->value.c_str(),
            cmd->value.length(),
            &acl,
            mode, 
            cmd->ttl,
            onStringCb,
            invoke
        );
    }
    else {
        rc = zoo_acreate(
            c->conn->zh,
            cmd->path.c_str(),
            cmd->value.c_str(),
            cmd->value.length(),
            &acl,
            mode,
            onStringCb,
            invoke
        );
    }

    if (!checkOk(rc, "zoo_acreate")) {
        delete invoke;
        auto rsp = std::make_shared<ZookRspData>();
        rsp->req = req;
        rsp->parser = std::make_shared<ZookParser>(rc, nullptr, nullptr, 0);
        onPushRsp(rsp, req->tData);
    }

    if (!zacl.empty()) {
        free(acl.data);
    }
}

void onDeleteCmd(ZookCorePtr c, ZookReqDataPtr req) {
    auto cmd = std::static_pointer_cast<DeleteCmd>(req->cmd);
    InvokeData* invoke = new InvokeData();
    invoke->req = req;
    invoke->c = c;

    int rc = zoo_adelete(c->conn->zh,
        cmd->path.c_str(),
        cmd->version,
        onVoidCb,
        invoke
    );

    if (!checkOk(rc, "zoo_adelete")) {
        delete invoke;
        auto rsp = std::make_shared<ZookRspData>();
        rsp->req = req;
        rsp->parser = std::make_shared<ZookParser>(rc, nullptr, nullptr, 0);
        onPushRsp(rsp, req->tData);
    }
}

void onExistsCmd(ZookCorePtr c, ZookReqDataPtr req) {
    auto cmd = std::static_pointer_cast<ExistsCmd>(req->cmd);
    InvokeData* invoke = new InvokeData();
    invoke->req = req;
    invoke->c = c;

    int rc = zoo_aexists(c->conn->zh,
        cmd->path.c_str(),
        0, /*no watch*/
        onStatCb,
        invoke
    );

    if (!checkOk(rc, "zoo_aexists")) {
        delete invoke;
        auto rsp = std::make_shared<ZookRspData>();
        rsp->req = req;
        rsp->parser = std::make_shared<ZookParser>(rc, nullptr, nullptr, 0);
        onPushRsp(rsp, req->tData);
    }
}

void onSetCmd(ZookCorePtr c, ZookReqDataPtr req) {
    auto cmd = std::static_pointer_cast<SetCmd>(req->cmd);
    InvokeData* invoke = new InvokeData();
    invoke->req = req;
    invoke->c = c;

    int rc = zoo_aset(
        c->conn->zh,
        cmd->path.c_str(),
        cmd->value.c_str(),
        cmd->value.length(),
        cmd->version,
        onStatCb,
        invoke
    );

    if (!checkOk(rc, "zoo_aset")) {
        delete invoke;
        auto rsp = std::make_shared<ZookRspData>();
        rsp->req = req;
        rsp->parser = std::make_shared<ZookParser>(rc, nullptr, nullptr, 0);
        onPushRsp(rsp, req->tData);
    }
}

void onGetAclCmd(ZookCorePtr c, ZookReqDataPtr req) {
    auto cmd = std::static_pointer_cast<GetAclCmd>(req->cmd);
    InvokeData* invoke = new InvokeData();
    invoke->req = req;
    invoke->c = c;

    int rc = zoo_aget_acl(
        c->conn->zh,
        cmd->path.c_str(),
        onAclCb,
        invoke
    );

    if (!checkOk(rc, "zoo_aget_acl")) {
        delete invoke;
        auto rsp = std::make_shared<ZookRspData>();
        rsp->req = req;
        rsp->parser = std::make_shared<ZookParser>(rc, nullptr, nullptr, 0);
        onPushRsp(rsp, req->tData);
    }
}

void onSetAclCmd(ZookCorePtr c, ZookReqDataPtr req) {
    auto cmd = std::static_pointer_cast<SetAclCmd>(req->cmd);
    InvokeData* invoke = new InvokeData();
    invoke->req = req;
    invoke->c = c;

    ACL_vector acl;
    std::vector<ZookParser::ZookAcl> zacl;
    aclParse(cmd->acls, acl, zacl);

    int rc = zoo_aset_acl(
        c->conn->zh,
        cmd->path.c_str(),
        cmd->version,
        &acl,
        onVoidCb,
        invoke
    );

    if (!checkOk(rc, "zoo_aset_acl")) {
        delete invoke;
        auto rsp = std::make_shared<ZookRspData>();
        rsp->req = req;
        rsp->parser = std::make_shared<ZookParser>(rc, nullptr, nullptr, 0);
        onPushRsp(rsp, req->tData);
    }

    if (!zacl.empty()) {
        free(acl.data);
    }
}

void onListCmd(ZookCorePtr c, ZookReqDataPtr req) {
    auto cmd = std::static_pointer_cast<ListCmd>(req->cmd);
    InvokeData* invoke = new InvokeData();
    invoke->req = req;
    invoke->c = c;

    int rc = zoo_aget_children(
        c->conn->zh,
        cmd->path.c_str(),
        0, /*no watch*/
        onStringsCb,
        invoke
    );

    if (!checkOk(rc, "zoo_aget_children")) {
        delete invoke;
        auto rsp = std::make_shared<ZookRspData>();
        rsp->req = req;
        rsp->parser = std::make_shared<ZookParser>(rc, nullptr, nullptr, 0);
        onPushRsp(rsp, req->tData);
    }
}

void opCmd(ZookCorePtr c, ZookReqDataPtr req) {
    auto cmd = req->cmd;
    if (get_cmd == cmd->cmdType) {
        opGetCmd(c, req);
    } 
    else if (create_cmd == cmd->cmdType) {
        onCreateCmd(c, req);
    } 
    else if (delete_cmd == cmd->cmdType) {
        onDeleteCmd(c, req);
    } 
    else if (exists_cmd == cmd->cmdType) {
        onExistsCmd(c, req);    
    } 
    else if (set_cmd == cmd->cmdType) {
        onSetCmd(c, req);
    } 
    else if (get_acl_cmd == cmd->cmdType) {
        onGetAclCmd(c, req);
    } 
    else if (set_acl_cmd == cmd->cmdType) {
        onSetAclCmd(c, req);
    } 
    else if (list_cmd == cmd->cmdType) {
        onListCmd(c, req);
    } 
    else {
        assert(false);
        zookLog("[error] invalid cmd: %d", cmd->cmdType);
        return;
    }
}


}
}

#endif