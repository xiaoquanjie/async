/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#ifdef USE_TRANS

#include "async/transaction/base_tick_transaction.h"

BaseTickTransaction::BaseTickTransaction() {
    m_work_state = en_idle_wstate;
    m_cur_time = 0;
}

void BaseTickTransaction::Tick(uint32_t cur_time) {
    if (m_work_state == en_working_state) {
        return;
    }

    m_cur_time = cur_time;
    m_work_state = en_working_state;

    this->Handle(0, 0, 0);
}

void BaseTickTransaction::InCoroutine() {
    OnTick(m_cur_time);
    m_work_state = en_idle_wstate;
}

#endif