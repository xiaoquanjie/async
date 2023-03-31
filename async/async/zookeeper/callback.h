/*----------------------------------------------------------------
// Copyright 2023
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

namespace async {
namespace zookeeper {

void onPushRsp(ZookRspDataPtr rsp, void* t);

void onTimeout(ZookReqDataPtr req, ZookCorePtr c, void* t);

// 连接
void onConnCb(zhandle_t* zh, int type, int state, const char *path, void* v);

//
void onAuthCb(int rc, const void *data);

// 数据回调
void onDataCb(int rc, const char *value, int value_len, const struct Stat *stat, const void *data);

void onStringCb(int rc, const char *value, const void *data);

void onVoidCb(int rc, const void *data);

void onStatCb(int rc, const struct Stat *stat, const void *data);

void onAclCb(int rc, struct ACL_vector *acl, struct Stat *stat, const void *data);

void onStringsCb(int rc, const struct String_vector *strings, const void *data);

void onWatchDataCb(int rc, const char *value, int value_len, const struct Stat *stat, const void *data);

// 数据监听
void onWatchCb(zhandle_t *zh, int type, int state, const char *path,void *watcherCtx);

void onWatchStringsCb(int rc, const struct String_vector *strings, const void *data);

// 子节点监听
void onWatchChildCb(zhandle_t *zh, int type, int state, const char *path,void *watcherCtx);

}
}