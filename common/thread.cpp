//
//  thread.cpp
//  common
//
//  Created by Damir Sagidullin on 25.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#include "thread.h"

using namespace mr;

#ifdef _WIN32

Thread::Thread() : m_thread(NULL)
{
}

Thread::~Thread()
{
	if (m_thread)
		::CloseHandle(m_thread);
}

DWORD WINAPI Thread::StaticThreadProc(void *pData)
{
	reinterpret_cast<Thread *>(pData)->ThreadProc();
	return 0;
}

bool Thread::Start()
{
	if (m_thread != NULL)
		return false;

	DWORD dwThreadId;
	m_thread = ::CreateThread(NULL, 0, &Thread::StaticThreadProc, this, 0, &dwThreadId);
	return m_thread != NULL;
}

void Thread::Join()
{
	if (m_thread != NULL)
	{
		::WaitForSingleObject(m_thread, INFINITE);
		m_thread = NULL;
	}
}

//void Thread::Sleep(int ms)
//{
//	::Sleep(ms);
//}

#else

Thread::Thread() : m_thread(NULL)
{
}

Thread::~Thread()
{
}

void *Thread::StaticThreadProc(void *pData)
{
	reinterpret_cast<Thread *>(pData)->ThreadProc();
	return NULL;
}

bool Thread::Start()
{
	if (m_thread != NULL)
		return false;

	return ::pthread_create(&m_thread, NULL, &Thread::StaticThreadProc, this) == 0;
}

void Thread::Join()
{
	if (m_thread != NULL)
	{
		::pthread_join(m_thread, NULL);
		m_thread = NULL;
	}
}

//void Thread::Sleep(int ms)
//{
//	::usleep(static_cast<useconds_t>(ms) * 1000);
//}

#endif