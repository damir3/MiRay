//
//  Mutex.h
//  MiRay/common
//
//  Created by Damir Sagidullin on 25.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

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

	void Lock();
	void Unlock();

	class Locker
	{
		Mutex	&m_mutex;

	public:
		Locker(Mutex &mutex) : m_mutex(mutex)
		{
			m_mutex.Lock();
		}

		~Locker()
		{
			m_mutex.Unlock();
		}
	};
};

}