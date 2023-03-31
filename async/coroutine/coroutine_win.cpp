/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#include "async/coroutine/coroutine.h"
#include "async/coroutine/tls.hpp"

#ifdef M_PLATFORM_WIN

#define gschedule tlsdata<_schedule_>::data()

void __stdcall pub_coroutine(LPVOID p) {
	_schedule_& schedule = gschedule;
	if (schedule._curco) {
		(schedule._curco->_function)(schedule._curco->_data);
		schedule._curco->_status = COROUTINE_DEAD;
		::SwitchToFiber(schedule._ctx);
	}
}

bool Coroutine::init(unsigned int stack_size) {
	return _init(stack_size);
}

bool Coroutine::init() {
	return _init(128 * 1204);
}

bool Coroutine::_init(unsigned int stack_size, bool pri_stack) {
	_schedule_& schedule = gschedule;
	if (schedule.init) {
		return true;
	}

	schedule.init = true;
	if (!schedule._ctx) {
		schedule._stack_size = stack_size;
		LPVOID ctx = ConvertThreadToFiberEx(0, FIBER_FLAG_FLOAT_SWITCH);
		if (!ctx) {
			DWORD error = GetLastError();
			return false;
		}
		else {
			schedule._ctx = ctx;
		}
	}
	return true;
}

int Coroutine::create(_coroutine_func_ routine, void* data) {
	init();
	_schedule_& schedule = gschedule;
	LPVOID ctx = ::CreateFiberEx(schedule._stack_size, 0, FIBER_FLAG_FLOAT_SWITCH, pub_coroutine, 0);
	if (ctx) {
		_coroutine_* co = _alloc_co_(schedule, routine, data);
		co->_ctx = ctx;
		return co->_id;
	}
	else {
		DWORD error = ::GetLastError();
		return M_INVALID_COROUTINE_ID;
	}
}

bool Coroutine::close() {
	_schedule_& schedule = gschedule;
	if (schedule._curco) {
		return false;
	}

	for (int i = 0; i < schedule._cap; ++i) {
		_coroutine_* co = schedule._co[i];
		if (co) {
			::DeleteFiber(co->_ctx);
			free(co);
		}
	}
	free(schedule._co);
	schedule.clear();
	::ConvertFiberToThread();
	return true;
}

void Coroutine::resume(int co_id) {
	_schedule_& schedule = gschedule;
	if (!_check_resume_(schedule, co_id)) {
		return;
	}

	_coroutine_* co = schedule._co[co_id - 1];
	if (!co) {
		return;
	}

	switch (co->_status) {
	case COROUTINE_READY:
	case COROUTINE_SUSPEND:
		co->_status = COROUTINE_RUNNING;
		schedule._curco = co;
		::SwitchToFiber(co->_ctx);
		if (co->_status == COROUTINE_DEAD) {
			::DeleteFiber(co->_ctx);
			free(co);
			schedule._co[co_id - 1] = 0;
			schedule._freeid.push_back(co_id);
		}
		schedule._curco = 0;
		break;
	}
}

void Coroutine::yield() {
	_schedule_& schedule = gschedule;
	if (schedule._curco) {
		schedule._curco->_status = COROUTINE_SUSPEND;
		::SwitchToFiber(schedule._ctx);
	}
}

bool Coroutine::destroy(int co_id) {
	_schedule_& schedule = gschedule;
	if (schedule._curco) {
		if (schedule._curco->_id == co_id) {
			return false;
		}
	}
	if (!_check_co_id(schedule, co_id)) {
		return false;
	}
	_coroutine_* co = schedule._co[co_id - 1];
	if (co) {
		::DeleteFiber(co->_ctx);
		free(co);
		schedule._co[co_id - 1] = 0;
		schedule._freeid.push_back(co_id);
	}
	return true;
}

#endif

