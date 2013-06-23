//
//  thread.h
//  common
//
//  Created by Damir Sagidullin on 25.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

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
	Mutex	m_mutexStartJoin;
	
protected:
	Thread();
	virtual ~Thread();

	virtual void ThreadProc() = 0;

public:
	bool Start();
	void Join();

//	static void Sleep(int ms);
};
	
}