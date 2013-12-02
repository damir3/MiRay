//
//  Mutex.cpp
//  MiRay/common
//
//  Created by Damir Sagidullin on 25.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#if __cplusplus < 201103L

#include "mutex.h"

using namespace mr;

#if defined(_WIN32)

Mutex::Mutex()
{
	::InitializeCriticalSection(&m_mutex);
}

Mutex::~Mutex()
{
	::DeleteCriticalSection(&m_mutex);
}

void Mutex::lock()
{
	::EnterCriticalSection(&m_mutex);
}

bool Mutex::try_lock()
{
	return (0 != ::TryEnterCriticalSection(&m_mutex));
}

void Mutex::unlock()
{
	::LeaveCriticalSection(&m_mutex);
}

#else

Mutex::Mutex()
{
	::pthread_mutex_init(&m_mutex, NULL);
}

Mutex::~Mutex()
{
	::pthread_mutex_destroy(&m_mutex);
}

void Mutex::lock()
{
	::pthread_mutex_lock(&m_mutex);
}

bool Mutex::try_lock()
{
	return (0 == ::pthread_mutex_trylock(&m_mutex));
}

void Mutex::unlock()
{
	::pthread_mutex_unlock(&m_mutex);
}

#endif

#endif