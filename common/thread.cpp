//
//  thread.cpp
//  common
//
//  Created by Damir Sagidullin on 25.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#if __cplusplus < 201103L && !(defined(_MSC_VER) && _MSC_VER >= 1700)

#include "thread.h"

using namespace mr;

#ifdef _WIN32

Thread::Thread(ThreadFunction func, void * pObj) : m_thread(NULL), m_func(func), m_pObj(pObj)
{
	MutexLockGuard lock(m_mutex);
	DWORD dwThreadId;
	m_thread = ::CreateThread(NULL, 0, &Thread::StaticThreadProc, this, 0, &dwThreadId);
}

Thread::~Thread()
{
	MutexLockGuard lock(m_mutex);
	if (m_thread)
		::CloseHandle(m_thread);
}

DWORD WINAPI Thread::StaticThreadProc(void *pData)
{
	Thread * pThis = reinterpret_cast<Thread *>(pData);
	pThis->m_func(pThis->m_pObj);
	return 0;
}

void Thread::join()
{
	MutexLockGuard lock(m_mutex);
	if (m_thread != NULL)
	{
		::WaitForSingleObject(m_thread, INFINITE);
		m_thread = NULL;
	}
}

#else

Thread::Thread(ThreadFunction func, void * pObj) : m_thread(NULL), m_func(func), m_pObj(pObj)
{
	MutexLockGuard lock(m_mutex);
	::pthread_create(&m_thread, NULL, &StaticThreadProc, this);
}

Thread::~Thread()
{
	join();
}

void *Thread::StaticThreadProc(void *pData)
{
	Thread * pThis = reinterpret_cast<Thread *>(pData);
	pThis->m_func(pThis->m_pObj);
	return NULL;
}

void Thread::join()
{
	MutexLockGuard lock(m_mutex);
	if (m_thread != NULL)
	{
		::pthread_join(m_thread, NULL);
		m_thread = NULL;
	}
}

#endif

#endif