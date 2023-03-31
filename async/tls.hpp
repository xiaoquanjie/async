/*----------------------------------------------------------------
// Copyright 2021
// All rights reserved.
//
// author: 404558965@qq.com (xiaoquanjie)
// github: https://github.com/xiaoquanjie/async
//----------------------------------------------------------------*/

#pragma once

#ifndef M_PLATFORM_WIN32
    #if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
        #define M_PLATFORM_WIN32 1
    #endif
#endif

#ifndef M_PLATFORM_WIN
    #if defined(M_PLATFORM_WIN32) || defined(WIN64)
        #define M_PLATFORM_WIN 1
    #endif
#endif

#ifdef M_PLATFORM_WIN
    #ifndef M_WIN32_LEAN_AND_MEAN  
        #define WIN32_LEAN_AND_MEAN
    #endif // end M_WIN32_LEAN_AND_MEAN
    #include <Windows.h>
#else
    #include <pthread.h>
    #include <ucontext.h>
#endif // end M_PLATFORM_WIN

namespace async {

#ifdef M_PLATFORM_WIN

template<typename T, int N = 0>
class tlsdata
{
public:
	struct _init_{
		DWORD _tkey;
		_init_(){
			_tkey = TlsAlloc();
			assert(_tkey != TLS_OUT_OF_INDEXES);
		}
		~_init_(){
			T* pv = (T*)TlsGetValue(_tkey);
			TlsFree(_tkey);
			delete pv;
		}
	};

	inline static T& data(){
		T* pv = 0;
		if (0 == (pv = (T*)TlsGetValue(_data._tkey))){
			pv = new T;
			TlsSetValue(_data._tkey, (void*)pv);
		}
		return *pv;
	}

protected:
	tlsdata(const tlsdata&);
	tlsdata& operator=(const tlsdata&);

private:
	static _init_ _data;
};

#else

#include <pthread.h>

template<typename T, int N = 0>
class tlsdata
{
public:
	struct _init_{
		pthread_key_t _tkey;
		_init_(){
			pthread_key_create(&_tkey, destructor);
		}
		~_init_(){
		}
	};

	union _point_ {
		T* p1;
		T* p2;
	};

	static T& data(){
		// 借用奇淫巧计，躲避gcc开启优化选项后编译的clobbered错误围杀
		_point_ p;
		if (0 == (p.p1 = (T*)pthread_getspecific(_data._tkey))) {
			p.p1 = new T;
			pthread_setspecific(_data._tkey, p.p1);
		}
		return *p.p2;
	}

protected:
	tlsdata(const tlsdata&);
	tlsdata& operator=(const tlsdata&);
	
	static void destructor(void* v) {
		T* pv = (T*)v;
		delete pv;
	}

private:
	static _init_ _data;
};

#endif

template<typename T, int N>
typename tlsdata<T, N>::_init_ tlsdata<T, N>::_data;


}