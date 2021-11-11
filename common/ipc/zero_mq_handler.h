/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include <vector>
#include <memory>

class ZeromqUnit;

// ZeromqUnit管理器, 要么全是router, 要么全是dealer，不能混合
class ZeromqHandler {
public:
    typedef std::shared_ptr<ZeromqUnit> ZeromqUnitPtr;

    bool listen(uint32_t id, const std::string& addr);    

    bool connect(uint32_t id, const std::string& addr);

    bool update(unsigned int);

    int sendData(uint32_t id, const std::string& data);

    int sendData(const std::string& data);
protected:
    bool addUnit(ZeromqUnitPtr ptr);

    // 数据回调
    virtual void onData(const std::string& data) {}
private:
    std::vector<ZeromqUnitPtr> m_unit_vec;
    int m_poll_id = 0;
};
