/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
//----------------------------------------------------------------*/

#pragma once

#include <stdlib.h>

template<typename T>
class svector{
protected:
	size_t _cap;
	size_t _size;
	T*  _head;
public:
	svector() {
		_cap = 100;
		_size = 0;
		_head = (T*)malloc(sizeof(T)*_cap);
	}

	svector(size_t cap) {
		_cap = cap;
		_size = 0;
		_head = (T*)malloc(sizeof(T)*_cap);
	}

	~svector() {
		clear();
		free(_head);
	}

	bool empty()const {
		return (_size == 0);
	}

	size_t size()const {
		return _size;
	}

	void swap(svector<T>& other) {
		size_t tmp = this->_cap;
		this->_cap = other._cap;
		other._cap = tmp;
		tmp = this->_size;
		this->_size = other._size;
		other._size = tmp;
		T* tmp2 = this->_head;
		this->_head = other._head;
		other._head = tmp2;
	}

	T& back() {
		return _head[_size-1];
	}

	void pop_back() {
		if (_size > 0) {
			_size--;
			T* p = &(this->_head[_size]);
			p->~T();
		}
	}

	void push_back(const T& t) {
		if (_size >= _cap) {
			this->_cap = this->_cap * 2;
			this->_head = (T*)realloc(this->_head, sizeof(T)*this->_cap);
		}
		T* p = &(_head[_size++]);
		new(p)T(t);
	}

	void clear() {
		for (size_t i = 0; i < _size; ++i) {
			T* p = &(this->_head[i]);
			p->~T();
		}
		_size = 0;
	}

	T& operator[](int i) {
		return _head[i];
	}
};

