//
//  Mutex.cpp
//  MiRay/common
//
//  Created by Damir Sagidullin on 25.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

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

void Mutex::Lock()
{
	::EnterCriticalSection(&m_mutex);
}

void Mutex::Unlock()
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

void Mutex::Lock()
{
	::pthread_mutex_lock(&m_mutex);
}

void Mutex::Unlock()
{
	::pthread_mutex_unlock(&m_mutex);
}

#endif