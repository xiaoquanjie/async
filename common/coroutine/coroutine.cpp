/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#include "common/coroutine/coroutine.hpp"
#include "common/coroutine/tls.hpp"
#include <cassert>

#define gschedule tlsdata<_schedule_>::data()

unsigned int Coroutine::curid() {
	_schedule_& schedule = gschedule;
	if (schedule._curco) {
		return schedule._curco->_id;
	}
	return M_MAIN_COROUTINE_ID;
}

void Coroutine::_alloc_schedule_(_schedule_& schedule) {
	if (!schedule._co) {
		schedule._nco = 0;
		schedule._cap = DEFAULT_COROUTINE;
		schedule._curco = NULL;
		schedule._co = (_coroutine_**)malloc(sizeof(_coroutine_*) * schedule._cap);
		assert(schedule._co);
		memset(schedule._co, 0, sizeof(_coroutine_*) * schedule._cap);
		schedule._freeid.clear();
	}
	else {
		if (schedule._nco < schedule._cap) {
			return;
		}
		int cap = schedule._cap * 2;
		schedule._co = (_coroutine_**)realloc(schedule._co, cap * sizeof(_coroutine_*));
		assert(schedule._co);
		memset(schedule._co + schedule._cap, 0, schedule._cap * sizeof(_coroutine_*));
		schedule._cap = cap;
	}
}

_coroutine_* Coroutine::_alloc_co_(_schedule_& schedule, _coroutine_func_ routine, void* data) {
	_coroutine_* co = (_coroutine_*)malloc(sizeof(_coroutine_));
	co->_function = routine;
	co->_data = data;
	co->_status = COROUTINE_READY;
	if (!schedule._freeid.empty()) {
		int id = schedule._freeid.back();
		schedule._freeid.pop_back();
		schedule._co[id - 1] = co;
		co->_id = id;
	}
	else {
		if (schedule._nco == schedule._cap) {
			_alloc_schedule_(schedule);
		}
		schedule._co[schedule._nco] = co;
		++schedule._nco;
		co->_id = schedule._nco;
	}
	return co;
}

bool Coroutine::_check_co_id(_schedule_& schedule, int co_id) {
	if (co_id <= M_MAIN_COROUTINE_ID) {
		return false;
	}
	if (co_id > schedule._cap) {
		return false;
	}
	return true;
}

bool Coroutine::_check_resume_(_schedule_& schedule, int co_id) {
	if (schedule._curco) {
		return false;
	}
	return _check_co_id(schedule, co_id);
}
