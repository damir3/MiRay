//
//  SoftwareRenderer.h
//  MiRay/rt
//
//  Created by Damir Sagidullin on 21.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

#include "../common/mutex.h"
#include "../common/thread.h"

namespace mr
{

class Image;

class SoftwareRenderer
{
	class RenderThread : public Thread
	{
		SoftwareRenderer &	m_renderer;
		volatile bool	m_bStop;

	public:
		RenderThread(SoftwareRenderer &	renderer) : m_renderer(renderer) {}

		void ThreadProc();
	};
	
	BVH & m_scene;

	Image *	m_pImage;
	RectI	m_rcRenderArea;
	Vec3	m_vEyePos;
	ColorF	m_bgColor;
	float	m_fFrameBlend;
	Vec3	m_vCamDelta[3];
	Vec2	m_dp;
	const Image * m_pEnvironmentMap;

	typedef TVec2<int>	Vec2I;
	Vec2I	m_pos;
	Vec2I	m_delta;

	float	m_fDistEpsilon;
	int		m_nMaxDepth;
	Mutex	m_mutex;

	std::vector<RenderThread *> m_renderThreads;

	bool GetNextArea(RectI & rc);
	void RenderArea(const RectI & rc) const;
	ColorF EnvironmentColor(const Vec3 & v) const;
	ColorF TraceRay(const Vec3 & v1, const Vec3 & v2, int nTraceDepth, const CollisionTriangle * pPrevTriangle) const;
	ColorF RenderPixel(const Vec2 & p) const;
	ColorF LookUpTexture(const Vec2 & p, const Vec2 & dp, const CollisionTriangle * pTriangle) const;

public:

	SoftwareRenderer(BVH & scene) : m_scene(scene) {}
	~SoftwareRenderer();

	void Render(Image & image, const RectI * pViewportRect,
				const Matrix & matCamera, const Matrix & matViewProj,
				const ColorF & bgColor, const Image * pEnvironmentMap,
				int numThreads, int nFrameNumber);

	void Join();
	void Interrupt();
};

}
