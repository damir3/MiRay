//
//  RenderThread.cpp
//  MiRay/ui
//
//  Created by Damir Sagidullin on 26.05.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#ifndef _WIN32
#include <unistd.h>
#endif

#include "RenderThread.h"

#include "../rt/SoftwareRenderer.h"
#include "../rt/OpenCLRenderer.h"

using namespace mr;

RenderThread::RenderThread()
	: m_mode(0)
	, m_pRenderer(NULL)
	, m_pOpenCLRenderer(NULL)
	, m_pRenderMap(NULL)
	, m_pBuffer(NULL)
	, m_pEnvironmentMap(NULL)
	, m_numCPU(0)
	, m_nFrameCount(0)
	, m_bStop(true)
	, m_bIsRenderMapUpdated(false)
{
}

RenderThread::~RenderThread()
{
	Stop();
	SAFE_DELETE(m_pRenderer);
	SAFE_DELETE(m_pOpenCLRenderer);
}

void RenderThread::SetRenderer(SoftwareRenderer * pRenderer)
{
	Stop();
	SAFE_DELETE(m_pRenderer);
	m_pRenderer = pRenderer;
}

void RenderThread::SetOpenCLRenderer(OpenCLRenderer * pRenderer)
{
	Stop();
	SAFE_DELETE(m_pOpenCLRenderer);
	m_pOpenCLRenderer = pRenderer;
}

void RenderThread::Start(int mode, Image & renderMap, Image & buffer,
						 const ColorF &bgColor, const Image *pEnvironmentMap,
						 const Matrix &matCamera, const Matrix &matViewProj)
{
	if (!m_pRenderer)
		return;

	m_mode = mode;
	m_pRenderMap = &renderMap;
	m_pBuffer = &buffer;
	m_bgColor = bgColor;
	m_pEnvironmentMap = pEnvironmentMap;
	m_matCamera = matCamera;
	m_matViewProj = matViewProj;
#ifdef _WIN32
	SYSTEM_INFO sysinfo;
	::GetSystemInfo(&sysinfo);
	int numCPU = static_cast<int>(sysinfo.dwNumberOfProcessors);
#else
	int numCPU = static_cast<int>(::sysconf(_SC_NPROCESSORS_ONLN));
#endif
	m_numCPU = numCPU;

	if (m_pRenderer && m_pRenderMap && m_pBuffer)
	{
		m_bStop = false;
		Thread::Start();
	}
}

void RenderThread::Stop()
{
	m_bStop = true;
	if (m_pRenderer && m_nFrameCount > 0)
	{
		m_bInterrupted = true;
		m_pRenderer->Interrupt();
	}
	Thread::Join();
}

void RenderThread::ThreadProc()
{
	m_nFrameCount = 0;
	m_bInterrupted = false;
	while (!m_bStop)
	{
		int nScale = m_nFrameCount < 2 ? (2 << (1 - m_nFrameCount)) : 1;
		RectI rcViewport(0, 0, m_pRenderMap->Width() / nScale, m_pRenderMap->Height() / nScale);
		double tm1 = Timer::GetSeconds();
		if (m_mode == 0)
		{
			m_pRenderer->ResetRayCounter();
			m_pRenderer->Render(*m_pBuffer, &rcViewport, m_matCamera, m_matViewProj,
								m_bgColor, m_pEnvironmentMap, m_numCPU, std::max(m_nFrameCount - 2, 0));
			m_pRenderer->Join();
		}
		else if (m_mode == 1)
		{
			m_pOpenCLRenderer->Render(*m_pBuffer, &rcViewport, m_matCamera, m_matViewProj,
									  m_bgColor, m_pEnvironmentMap, m_numCPU, std::max(m_nFrameCount - 2, 0));
		}
		m_nFrameCount++;
		double tm2 = Timer::GetSeconds();

		if (m_mode == 0)
			printf("%d: %dx%d (%d threads) %.2f ms, %.3f mrps\n", m_nFrameCount - 1, rcViewport.Width(), rcViewport.Height(), m_numCPU, (tm2 - tm1) * 1000.0,
				   m_pRenderer->RaysCounter() * 1e-6 / (tm2 - tm1));
		else
			printf("%d: %dx%d %f ms\n", m_nFrameCount - 1, rcViewport.Width(), rcViewport.Height(), (tm2 - tm1) * 1000.0);

		if (!m_bInterrupted)
		{// update render map
			Mutex::Locker locker(m_mutex);
			memcpy(m_pRenderMap->Data(), m_pBuffer->Data(), rcViewport.Height() * m_pRenderMap->Width() * m_pRenderMap->PixelSize());
			m_rcRenderMap = rcViewport;
			m_bIsRenderMapUpdated = true;

//			m_pRenderMap->SetPixel(0, 0, ColorF(1.f, 0.f, 0.f, 1.f));
//			m_pRenderMap->SetPixel(rcViewport.Width() - 1, 0, ColorF(1.f, 0.f, 0.f, 1.f));
//			m_pRenderMap->SetPixel(0, rcViewport.Height() - 1, ColorF(1.f, 0.f, 0.f, 1.f));
//			m_pRenderMap->SetPixel(rcViewport.Width() - 1, rcViewport.Height() - 1, ColorF(1.f, 0.f, 0.f, 1.f));
		}
	}
}

RectI RenderThread::LockRenderMap()
{
	m_mutex.Lock();
	m_bIsRenderMapUpdated = false;
	return m_rcRenderMap;
}

void RenderThread::UnlockRenderMap()
{
	m_mutex.Unlock();
}
