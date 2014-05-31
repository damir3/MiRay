//
//  RenderThread.h
//  MiRay/ui
//
//  Created by Damir Sagidullin on 26.05.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

#include "../common/mutex.h"
#include "../common/thread.h"

namespace mr
{

class SoftwareRenderer;
class OpenCLRenderer;

class RenderThread
{
	SoftwareRenderer *	m_pRenderer;
	OpenCLRenderer *	m_pOpenCLRenderer;
	int		m_mode;
	Image *	m_pRenderMap;
	Image *	m_pBuffer;
	Matrix	m_matCamera;
	Matrix	m_matViewProj;
	int		m_numCPU;
	RectI	m_rcRenderMap;
	volatile bool	m_bStop;
	volatile bool	m_bIsRenderMapUpdated;
	volatile int	m_nFrameCount;
	volatile double	m_fFramesRenderTime;
	Mutex	m_mutex;
	std::unique_ptr<Thread>	m_thread;

public:
	RenderThread();
	~RenderThread();

	SoftwareRenderer * Renderer() { return m_pRenderer; }
	void SetRenderer(SoftwareRenderer * pRenderer);
	void SetOpenCLRenderer(OpenCLRenderer * pRenderer);

	void Start(int mode, Image & renderMap, Image & buffer, const Matrix & matCamera, const Matrix & matViewProj);
	void Stop();

	void ThreadFunc();

	int FramesCount() const { return m_nFrameCount - 2; }
	double FramesRenderTime() const { return m_fFramesRenderTime; }
	bool IsRenderMapUpdated() const { return m_bIsRenderMapUpdated; }
	RectI LockRenderMap();
	void UnlockRenderMap();
};

}
