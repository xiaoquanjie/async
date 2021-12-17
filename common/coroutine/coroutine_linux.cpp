/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#include "common/coroutine/coroutine.h"
#include "common/coroutine/tls.hpp"

#ifndef M_PLATFORM_WIN

#define gschedule tlsdata<_schedule_>::data()

void pub_coroutine() {
	_schedule_& schedule = gschedule;
	if (schedule._curco) {
		(schedule._curco->_function)(schedule._curco->_data);
		schedule._curco->_status = COROUTINE_DEAD;
		swapcontext(&schedule._curco->_ctx, &schedule._ctx);
	}
}

void save_stack(_coroutine_* c, char* top) {
	char dummy = 0;
	assert(top - &dummy <= M_COROUTINE_STACK_SIZE);
	c->_size = top - &dummy;
	if (c->_cap < c->_size || (c->_cap - c->_size > 1024)) {
		free(c->_stack);
		c->_cap = c->_size;
		c->_stack = (char*)malloc(c->_cap);
	}
	memcpy(c->_stack, &dummy, c->_size);
}

////////////////////////////////////////////////////////////

bool Coroutine::init(unsigned int stack_size) {
	return _init(stack_size, true);
}

bool Coroutine::init() {
	return _init(0, false);
}

bool Coroutine::_init(unsigned int stack_size, bool pri_stack) {
	_schedule_& schedule = gschedule;
	if (schedule.init) {
		return true;
	}

	schedule.init = true;
	schedule._pri_stack = pri_stack;
	schedule._stack_size = stack_size;
	return true;
}

int Coroutine::create(_coroutine_func_ routine, void* data) {
	init();
	_schedule_& schedule = gschedule;
	if (schedule._stack == 0) {
		return M_INVALID_COROUTINE_ID;
	}

	_coroutine_* co = _alloc_co_(schedule, routine, data);
	getcontext(&co->_ctx);
	co->_ctx.uc_link = &schedule._ctx;
	co->_size = 0;

	if (schedule._pri_stack) {
		co->_cap = schedule._stack_size;
		co->_stack = (char*)malloc(co->_cap);
		co->_ctx.uc_stack.ss_sp = co->_stack;
		co->_ctx.uc_stack.ss_size = co->_cap;
		}
	else {
		co->_stack = 0;
		co->_cap = 0;
		co->_ctx.uc_stack.ss_sp = schedule._stack;
		co->_ctx.uc_stack.ss_size = M_COROUTINE_STACK_SIZE;
	}
	makecontext(&co->_ctx, pub_coroutine, 0);
	return co->_id;
}

bool Coroutine::close() {
	_schedule_& schedule = gschedule;
	if (schedule._curco) {
		return false;
	}

	if (!schedule._curco) {
		for (int i = 0; i < schedule._cap; ++i) {
			_coroutine_* co = schedule._co[i];
			if (co) {
				free(co->_stack);
				free(co);
			}
		}
	}

	free(schedule._co);
	schedule.clear();
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
	case COROUTINE_SUSPEND:
		if (!schedule._pri_stack)
			memcpy(schedule._stack + M_COROUTINE_STACK_SIZE - co->_size, co->_stack, co->_size);
	case COROUTINE_READY:
		co->_status = COROUTINE_RUNNING;
		schedule._curco = co;
		swapcontext(&schedule._ctx, &co->_ctx);
		if (co->_status == COROUTINE_DEAD) {
			free(co->_stack);
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
		_coroutine_* co = schedule._curco;
		if (!schedule._pri_stack)
			save_stack(co, schedule._stack + M_COROUTINE_STACK_SIZE);
		co->_status = COROUTINE_SUSPEND;
		swapcontext(&co->_ctx, &schedule._ctx);
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
		free(co->_stack);
		free(co);
		schedule._co[co_id - 1] = 0;
		schedule._freeid.push_back(co_id);
	}
	return true;
}

#endif
