//
//  thread.h
//  common
//
//  Created by Damir Sagidullin on 25.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

#ifndef _WIN32
#include <pthread.h>
//#include <unistd.h>
#endif

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