//
//  thread.h
//  common
//
//  Created by Damir Sagidullin on 25.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

#if __cplusplus < 201103L

#include "mutex.h"

namespace mr
{
	class Thread
	{
#ifdef _WIN32
		static DWORD WINAPI StaticThreadProc(void *);		

		HANDLE m_thread;
#else
		static void * StaticThreadProc(void *);

		pthread_t m_thread;
#endif
		typedef void (*ThreadFunction)(void *);
		ThreadFunction	m_func;
		void * const 	m_pObj;
		Mutex			m_mutex;
		
	public:
		Thread(ThreadFunction func, void * pObj);
		~Thread();
		
		void join();
	};
}

#else

#include <thread>

namespace mr
{
	typedef std::thread		Thread;
}

#endif
