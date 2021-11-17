/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include "common/transaction/base_transaction.h"
#include <stdint.h>
#include <list>
#include <typeinfo>

namespace trans_mgr {

uint32_t genTransId();

// 事务桶
class TransactionBucket {
public:
    TransactionBucket(uint32_t req_cmd_id, uint32_t rsp_cmd_id) {
        m_req_cmd_id = req_cmd_id;
        m_rsp_cmd_id = rsp_cmd_id; 
    }

    virtual ~TransactionBucket() {}

    virtual BaseTransaction* Create() = 0;

    virtual void Recycle(BaseTransaction*) = 0;

    virtual uint32_t Size() const = 0;

    virtual const char* TransName() = 0;

    uint32_t ReqCmdId() {
        return m_req_cmd_id;
    }

    uint32_t RspCmdId() {
        return m_rsp_cmd_id;
    }
protected:
    uint32_t m_req_cmd_id;
    uint32_t m_rsp_cmd_id; 
};

template<typename TransactionType>
class TransactionBucketImpl : public TransactionBucket {
public:
    TransactionBucketImpl(uint32_t req_cmd_id, uint32_t rsp_cmd_id) 
		: TransactionBucket(req_cmd_id, rsp_cmd_id) {
	}

    ~TransactionBucketImpl() {
        for (auto iter = m_trans_vec.begin(); iter != m_trans_vec.end(); ++iter) {
			delete (*iter);
		}
		m_trans_vec.clear();
    }

    BaseTransaction* Create() {
        TransactionType* trans = 0;
        if (m_trans_vec.empty()) {
            trans = new TransactionType();
        }
        else {
            trans = m_trans_vec.back();
            m_trans_vec.pop_back();
            new(trans)TransactionType();
        }

        trans->SetReqCmdId(m_req_cmd_id);
        trans->SetRspCmdId(m_rsp_cmd_id);
        trans->SetTransId(genTransId());
        return trans;
    }

    void Recycle(BaseTransaction* trans) {
        TransactionType* t = dynamic_cast<TransactionType*>(trans);
        if (m_trans_vec.size() > 20) {
			delete t;
		}
		else {
			t->~TransactionType();
			m_trans_vec.push_back(t);
		}
    }

    uint32_t Size() const {
        return m_trans_vec.size();
    }

    virtual const char* TransName() {
        return typeid(TransactionType).name();
    }

protected:
    std::list<TransactionType*> m_trans_vec;
};

}