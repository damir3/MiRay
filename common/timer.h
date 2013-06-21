//
//  timer.h
//  MiRay/common
//
//  Created by Damir Sagidullin on 25.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

namespace mr
{

class Timer
{
#if defined(_WIN32) || defined(__APPLE__)

	static Timer g_timer;
	double m_fClockToSeconds;

	double GetTime();

#endif

public:

#if defined(_WIN32) || defined(__APPLE__)

	Timer();

	static double GetSeconds() { return g_timer.GetTime(); }

#else

	static double GetSeconds();

#endif
};

}