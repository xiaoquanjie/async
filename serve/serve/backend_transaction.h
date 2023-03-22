/*----------------------------------------------------------------
// Copyright 2022
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#include "common/transaction/transaction_mgr.h"
#include "serve/serve/base.h"
#include "serve/serve/backend.h"
#include <assert.h>

template<typename RequestType, typename RespondType=NullRespond>
class BackendTransaction : public Transaction<RequestType, RespondType> {
public:
    using BaseTrans = Transaction<RequestType, RespondType>;

    bool ParsePacket(const char* packet, uint32_t /*packet_size*/) override {
        const BackendMsg* msg = (const BackendMsg*)packet;
        if (!BaseTrans::m_request.ParseFromString(msg->data)) {
            assert(false);
            return false;
        }

        this->mHeader = msg->header;
        return true;
    }

    bool OnAfter() override {
        if (BaseTrans::RspCmdId() != 0) {
            sendBack(BaseTrans::m_respond);
        }
        return true;
    }

    void sendBack(RespondType &data) {
        backend::notifyBack(mHeader.srcWorldId,
                            mHeader.targetId,
                            data, 
                            BaseTrans::RspCmdId(),
                            mHeader.frontSeqNo,
                            mHeader.reqSeqId,
                            BaseTrans::m_return_value,
                            0);
    }

protected:
    BackendHeader mHeader;
};

