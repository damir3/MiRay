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
	, m_numCPU(1)
	, m_nFrameCount(0)
	, m_fFramesRenderTime(0.0)
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

static void StaticThreadFunc(void * pRenderThread) { reinterpret_cast<RenderThread *>(pRenderThread)->ThreadFunc(); }

void RenderThread::Start(int mode, Image & renderMap, Image & buffer, const Matrix &matCamera, const Matrix &matViewProj)
{
	if (!m_pRenderer)
		return;

	m_mode = mode;
	m_pRenderMap = &renderMap;
	m_pBuffer = &buffer;
	m_matCamera = matCamera;
	m_matViewProj = matViewProj;
#ifdef _WIN32
	SYSTEM_INFO sysinfo;
	::GetSystemInfo(&sysinfo);
	int numCPU = static_cast<int>(sysinfo.dwNumberOfProcessors);
#else
	int numCPU = static_cast<int>(::sysconf(_SC_NPROCESSORS_ONLN));
#endif
	m_numCPU = std::max(numCPU - 1, 1);

	if (m_pRenderer && m_pRenderMap && m_pBuffer)
	{
		m_bStop = false;
		m_thread.reset(new Thread(&StaticThreadFunc, this));
	}
}

void RenderThread::Stop()
{
	m_bStop = true;
	if (m_pRenderer && m_nFrameCount > 0)
		m_pRenderer->Interrupt();

	if (m_thread)
	{
		m_thread->join();
		m_thread.reset();
	}
}

void RenderThread::ThreadFunc()
{
	m_nFrameCount = 0;
	m_fFramesRenderTime = 0.0;
	while (!m_bStop)
	{
		int nScale = m_nFrameCount < 2 ? (2 << (1 - m_nFrameCount)) : 1;
		RectI rcViewport(0, 0, m_pRenderMap->Width() / nScale, m_pRenderMap->Height() / nScale);
		double tm1 = Timer::GetSeconds();
		if (m_mode == 0)
		{
			m_pRenderer->ResetRayCounter();
			m_pRenderer->Render(*m_pBuffer, &rcViewport, m_matCamera, m_matViewProj,
								m_nFrameCount > 2 ? Vec2(frand(), frand()) : Vec2(0.5f, 0.5f),
								m_numCPU, std::max(m_nFrameCount - 2, 0));
			m_pRenderer->Join();
		}
		else if (m_mode == 1 && m_pOpenCLRenderer)
		{
			m_pOpenCLRenderer->Render(*m_pBuffer, &rcViewport, m_matCamera, m_matViewProj, std::max(m_nFrameCount - 2, 0));
		}
		double tm2 = Timer::GetSeconds();

		if (!m_bStop || !m_nFrameCount)
		{// update render map
//			if (m_mode == 0)
//				printf("%d: %dx%d (%d threads) %.2f ms, %.3f mrps\n", m_nFrameCount, rcViewport.Width(), rcViewport.Height(),
//					   m_numCPU, (tm2 - tm1) * 1000.0, m_pRenderer->RaysCounter() * 1e-6 / (tm2 - tm1));
//			else
//				printf("%d: %dx%d %f ms\n", m_nFrameCount, rcViewport.Width(), rcViewport.Height(), (tm2 - tm1) * 1000.0);

			m_mutex.lock();
			memcpy(m_pRenderMap->Data(), m_pBuffer->Data(), rcViewport.Height() * m_pRenderMap->Width() * m_pRenderMap->PixelSize());
			m_rcRenderMap = rcViewport;
			m_bIsRenderMapUpdated = true;
			m_mutex.unlock();

//			m_pRenderMap->SetPixel(0, 0, ColorF(1.f, 0.f, 0.f, 1.f));
//			m_pRenderMap->SetPixel(rcViewport.WidthØ() - 1, 0, ColorF(1.f, 1.f, 0.f, 1.f));
//			m_pRenderMap->SetPixel(0, rcViewport.Height() - 1, ColorF(0.f, 1.f, 0.f, 1.f));
//			m_pRenderMap->SetPixel(rcViewport.Width() - 1, rcViewport.Height() - 1, ColorF(0.f, 0.f, 1.f, 1.f));
		}

		m_nFrameCount++;
		if (m_nFrameCount > 2)
			m_fFramesRenderTime += tm2 - tm1;
	}
}

RectI RenderThread::LockRenderMap()
{
	m_mutex.lock();
	m_bIsRenderMapUpdated = false;
	return m_rcRenderMap;
}

void RenderThread::UnlockRenderMap()
{
	m_mutex.unlock();
}
