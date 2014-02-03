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

class IImage;
class ILight;

class SoftwareRenderer
{
	BVH &	m_scene;
	std::vector<ILight *>	m_lights;

	IImage *m_pImage;
	RectI	m_rcRenderArea;
	Vec3	m_vEyePos;
	ColorF	m_bgColor;
	float	m_fFrameBlend;
	Vec3	m_vCamDelta[3];
	Vec2	m_dp;
	const IImage *m_pEnvironmentMap;

	int		m_nAreaCounter;
	int		m_numAreasX;
	int		m_numAreas;
	typedef TVec2<int>	Vec2I;
	Vec2I	m_delta;
	Vec2	m_random;
	Matrix	m_matRandom;

	float	m_fDistEpsilon;
	float	m_fRayLength;
	int		m_nMaxDepth;
	Mutex	m_mutex;

	volatile uint32 m_nRayCounter;

	std::vector<Thread *> m_renderThreads;

	bool GetNextArea(RectI & rc);
	void RenderArea(const RectI & rc) const;

	struct sResult
	{
		Vec3 color;
		Vec3 opacity;
		sResult(const Vec3 & c, const Vec3 & o) : color(c), opacity(o) {}
		sResult(const ColorF & c) : color(c.r, c.g, c.b), opacity(c.a) {}
	};

	Vec3 RandomDirection(const Vec3 & normal) const;
	ColorF EnvironmentColor(const Vec3 & v) const;
	sResult TraceRay(const Vec3 & v1, const Vec3 & v2, int nTraceDepth, const CollisionTriangle * pPrevTriangle) const;
	ColorF RenderPixel(const Vec2 & p) const;
	ColorF LookUpTexture(const Vec2 & p, const Vec2 & dp, const CollisionTriangle * pTriangle) const;

	static void ThreadFunc(void * pRenderer);

public:

	SoftwareRenderer(BVH & scene);
	~SoftwareRenderer();
	
	uint32 RaysCounter() const { return m_nRayCounter; }
	void ResetRayCounter() { m_nRayCounter = 0; }

	void SetLights(size_t num, ILight ** ppLights);

	void Render(IImage & image, const RectI * pViewportRect,
				const Matrix & matCamera, const Matrix & matViewProj,
				const ColorF & bgColor, const IImage * pEnvironmentMap,
				int numThreads, int nFrameNumber);

	void Join();
	void Interrupt();
};

}
