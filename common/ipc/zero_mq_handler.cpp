/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#ifdef USE_IPC

#include "common/ipc/zero_mq_handler.h"
#include "common/ipc/zero_mq_unit.h"
#include <zmq.h>
#include <assert.h>
#include <algorithm>

bool ZeromqHandler::update(unsigned int) {
    if (m_unit_vec.empty()) {
        return false;
    }

    // 最大路由数量限制为512
	size_t size = m_unit_vec.size();
	if (size > 512) {
		throw std::string("error");
		return false;
	}

    zmq_pollitem_t items[512];
	for (size_t idx = 0; idx < size; ++idx) {
		items[idx].socket = m_unit_vec[idx]->getSock();
		items[idx].events = ZMQ_POLLIN;
		items[idx].fd = 0;
	}

    int rc = zmq_poll(items, size, 0);
	if (rc > 0) {
		for (size_t idx = 0; idx < size; ++idx) {
			if (items[idx].revents & ZMQ_POLLIN) {
				do {
                    std::string identify;
					std::string data;
					if (m_unit_vec[idx]->recvData(identify, data) <= 0) {
						break;
					}
					onData(m_unit_vec[idx]->uniqueId(), identify, data);
				} while (true);
			}
		}
	}

	return (rc > 0);
}

bool ZeromqHandler::addUnit(ZeromqUnitPtr ptr) {
    // 按id进行排序
    m_unit_vec.push_back(ptr);
    // std::sort(m_unit_vec.begin(), m_unit_vec.end(), [](ZeromqUnitPtr ptr1, ZeromqUnitPtr ptr2) {
    //     return ptr1->zeromqId() < ptr2->zeromqId();
    // });
    return true;
}

/////////////////////////////////////////////////////

uint64_t ZeromqRouterHandler::listen(uint32_t id, const std::string& addr) {
    auto ptr = std::make_shared<ZeromqRouter>(id, addr);
    if (!ptr->listen()) {
        assert(false);
        return 0;
    }

    addUnit(ptr);
    return ptr->uniqueId();
}

int ZeromqRouterHandler::sendData(uint64_t unique_id, uint32_t other_id, const std::string& data) {
    for (auto& item : m_unit_vec) {
        if (item->uniqueId() == unique_id) {
            typedef std::shared_ptr<ZeromqRouter> ZeromqRouterPtr;
            ZeromqRouterPtr ptr = std::dynamic_pointer_cast<ZeromqRouter>(item);
            return ptr->sendData(other_id, data);
        }
    }

    assert(false);
    return 0;
}

int ZeromqRouterHandler::sendData(uint64_t unique_id, const std::string& identify, const std::string& data) {
    for (auto& item : m_unit_vec) {
        if (item->uniqueId() == unique_id) {
            typedef std::shared_ptr<ZeromqRouter> ZeromqRouterPtr;
            ZeromqRouterPtr ptr = std::dynamic_pointer_cast<ZeromqRouter>(item);
            return ptr->sendData(identify, data);
        }
    }

    assert(false);
    return 0; 
}

/////////////////////////////////////////////////////

uint64_t ZeromqDealerHandler::connect(uint32_t id, const std::string& addr) {
    auto ptr = std::make_shared<ZeromqDealer>(id, addr);
    if (!ptr->connect()) {
        assert(false);
        return 0;
    }

    addUnit(ptr);
    return ptr->uniqueId();
}

int ZeromqDealerHandler::sendData(uint64_t unique_id, const std::string& data) {
    for (auto& item : m_unit_vec) {
        if (item->uniqueId() == unique_id) {
            typedef std::shared_ptr<ZeromqDealer> ZeromqDealerPtr;
            ZeromqDealerPtr ptr = std::dynamic_pointer_cast<ZeromqDealer>(item);
            return ptr->sendData(data);
        }
    }

    assert(false);
    return 0; 
}

#endif