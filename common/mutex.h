//
//  Mutex.h
//  MiRay/common
//
//  Created by Damir Sagidullin on 25.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

#if __cplusplus < 201103L && !(defined(_MSC_VER) && _MSC_VER >= 1700)

#ifndef _WIN32
#include <pthread.h>
#endif

namespace mr
{

	class Mutex
	{
#ifdef _WIN32
		CRITICAL_SECTION m_mutex;
#else
		pthread_mutex_t m_mutex;
#endif
		
	public:
		Mutex();
		~Mutex();
		
		void lock();
		bool try_lock();
		void unlock();
	};

	class MutexLockGuard
	{
		Mutex	&m_mutex;
		
	public:
		MutexLockGuard(Mutex &mutex) : m_mutex(mutex)
		{
			m_mutex.lock();
		}
		
		~MutexLockGuard()
		{
			m_mutex.unlock();
		}
	};
}

#else

#include <mutex>

namespace mr
{
	typedef std::mutex						Mutex;
	typedef std::lock_guard<std::mutex>		MutexLockGuard;
}

#endif
