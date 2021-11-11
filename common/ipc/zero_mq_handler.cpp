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

bool ZeromqHandler::listen(uint32_t id, const std::string& addr) {
    auto ptr = std::make_shared<ZeromqUnit>(id, addr);
    if (!ptr->listen()) {
        assert(false);
        return false;
    }

    addUnit(ptr);
    return true;
}

bool ZeromqHandler::connect(uint32_t id, const std::string& addr) {
    auto ptr = std::make_shared<ZeromqUnit>(id, addr);
    if (!ptr->connect()) {
        assert(false);
        return false;
    }

    addUnit(ptr);
    return true;
}

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
	//zmq_pollitem_t* items = (zmq_pollitem_t*)malloc(sizeof(zmq_pollitem_t) * size);
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
					std::string data;
					if (m_unit_vec[idx]->recvData(data) <= 0) {
						break;
					}
					onData(data);
				} while (true);
			}
		}
	}

	return (rc > 0);
}

bool ZeromqHandler::addUnit(ZeromqUnitPtr ptr) {
    // 按id进行排序
    m_unit_vec.push_back(ptr);
    std::sort(m_unit_vec.begin(), m_unit_vec.end(), [](ZeromqUnitPtr ptr1, ZeromqUnitPtr ptr2) {
        return ptr1->id() < ptr2->id();
    });
    return true;
}

int ZeromqHandler::sendData(uint32_t id, const std::string& data) {
    if (m_unit_vec.empty()) {
        return 0;
    }
    // 轮询一个实例发
    int idx = (m_poll_id++) % m_unit_vec.size();
    return m_unit_vec[idx]->sendData(id, data);
}

int ZeromqHandler::sendData(const std::string& data) {
    if (m_unit_vec.empty()) {
        return 0;
    }
    // 轮询一个实例发
    int idx = (m_poll_id++) % m_unit_vec.size();
    return m_unit_vec[idx]->sendData(data);
}

#endif