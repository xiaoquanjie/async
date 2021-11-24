/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include "common/ipc/zero_mq_unit.h"
#include <vector>
#include <memory>

// ZeromqUnit管理器
class ZeromqHandler {
public:
    typedef std::shared_ptr<ZeromqUnit> ZeromqUnitPtr;

    bool update(unsigned int);

protected:
    bool addUnit(ZeromqUnitPtr ptr);

    // 数据回调
    // @unique_id代表一个ZeromqUnit的唯一id
    // @identify表示的是对方连接的标识符
    // @data是数据包
    virtual void onData(uint64_t /*unique_id*/, std::string& /*identify*/, const std::string& /*data*/) {}

protected:
    std::vector<ZeromqUnitPtr> m_unit_vec;
};

/////////////////////////////////////////////////////

// router, 可以监听多个相同功能的端口（但一般不会这么做)
class ZeromqRouterHandler : public ZeromqHandler {
public:
    // 返回唯一id, 失败返回0
    uint64_t listen(uint32_t id, const std::string& addr);

    // @unique_id代表一个ZeromqUnit的唯一id
    // @other_id对方的id
    // @data数据
    int sendData(uint64_t unique_id, uint32_t other_id, const std::string& data);

    int sendData(uint64_t unique_id, const std::string& identify, const std::string& data);
};

/////////////////////////////////////////////////////

// dealer, 可以连接多个相同功能的router
class ZeromqDealerHandler : public ZeromqHandler {
public:
    // 返回唯一id, 失败返回0
    uint64_t connect(uint32_t id, const std::string& addr);    

    // @unique_id代表一个ZeromqUnit的唯一id
    // @data数据
    int sendData(uint64_t unique_id, const std::string& data);
};