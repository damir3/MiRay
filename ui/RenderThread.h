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

class RenderThread : public Thread
{
	SoftwareRenderer *	m_pRenderer;
	OpenCLRenderer *	m_pOpenCLRenderer;
	int		m_mode;
	Image *	m_pRenderMap;
	Image *	m_pBuffer;
	ColorF	m_bgColor;
	const Image * m_pEnvironmentMap;
	Matrix	m_matCamera;
	Matrix	m_matViewProj;
	int		m_numCPU;
	RectI	m_rcRenderMap;
	volatile bool	m_bStop;
	volatile bool	m_bIsRenderMapUpdated;
	volatile int	m_nFrameCount;
	volatile bool	m_bInterrupted;
	Mutex	m_mutex;

public:
	RenderThread();
	~RenderThread();

	SoftwareRenderer * Renderer() { return m_pRenderer; }
	void SetRenderer(SoftwareRenderer * pRenderer);
	void SetOpenCLRenderer(OpenCLRenderer * pRenderer);

	void Start(int mode, Image & renderMap, Image & buffer, const ColorF & bgColor, const Image * pEnvironmentMap,
			   const Matrix & matCamera, const Matrix & matViewProj);
	void Stop();

	void ThreadProc();

	bool IsRenderMapUpdated() const { return m_bIsRenderMapUpdated; }
	RectI LockRenderMap();
	void UnlockRenderMap();
};

}
