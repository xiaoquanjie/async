/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include "common/ipc/zero_mq_unit.h"
#include <vector>
#include <memory>

namespace ipc {

// ZeromqUnit管理器
class ZeromqHandler {
public:
    virtual ~ZeromqHandler() {}

    typedef std::shared_ptr<ZeromqUnit> ZeromqUnitPtr;

    bool update(unsigned int);

protected:
    bool addUnit(ZeromqUnitPtr ptr);

    // 数据回调
    // @uniqueId代表一个ZeromqUnit的唯一id
    // @identify表示的是对方连接的标识符
    // @data是数据包
    virtual void onData(uint64_t /*uniqueId*/, uint32_t /*otherIdentify*/, const std::string& /*data*/) {}

protected:
    std::vector<ZeromqUnitPtr> m_unit_vec;
};

/////////////////////////////////////////////////////

// router, 可以监听多个相同功能的端口（但一般不会这么做)
class ZeromqRouterHandler : public ZeromqHandler {
public:
    // 返回唯一id, 失败返回0
    uint64_t listen(const std::string& addr);

    // @uniqueId代表一个ZeromqUnit的唯一id
    // @otherIdentify对方的id
    // @data数据
    int sendData(uint64_t uniqueId, uint32_t otherIdentify, const std::string& data);
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

}