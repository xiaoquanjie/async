/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include "async/transaction/base_transaction.h"

class BaseTickTransaction : public BaseTransaction {
public:
    enum {
        en_idle_wstate = 0,
        en_working_state,
    };

    BaseTickTransaction();

    void Tick(uint32_t cur_time);

    virtual bool ParsePacket(const char*, uint32_t) override { return false; }
protected:
    virtual void InCoroutine() override;

    virtual void OnTick(uint32_t) = 0;
private:
    uint32_t m_work_state;
    uint32_t m_cur_time;
};