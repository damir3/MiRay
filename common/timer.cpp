//
//  timer.cpp
//  common
//
//  Created by Damir Sagidullin on 25.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#include "timer.h"

#if defined(_WIN32)

using namespace mr;

Timer	Timer::g_timer;

Timer::Timer() : m_fClockToSeconds(0.0)
{
	LARGE_INTEGER frequency;
	if (::QueryPerformanceFrequency(&frequency) && (frequency.QuadPart > 0))
		m_fClockToSeconds = (1.0 / frequency.QuadPart);
}

double Timer::GetTime()
{
	LARGE_INTEGER counter;
	if (m_fClockToSeconds == 0.0)
		return GetTickCount() * 1000.0;

	::QueryPerformanceCounter(&counter);
	return counter.QuadPart * m_fClockToSeconds;
}

#elif defined(__APPLE__)

#include <mach/mach_time.h>

using namespace mr;

Timer	Timer::g_timer;

Timer::Timer() : m_fClockToSeconds(0.0)
{
	mach_timebase_info_data_t data;
	::mach_timebase_info(&data);
	m_fClockToSeconds = 1e-9 * data.numer / data.denom;
}

double Timer::GetTime()
{
	return ::mach_absolute_time() * m_fClockToSeconds;
}

#else

#include <time.h>

using namespace mr;

double Timer::GetSeconds()
{
	return ::clock() * (1.0 / CLOCKS_PER_SEC);
}


#endif
