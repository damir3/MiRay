//
//  SoftwareRenderer.h
//  MiRay/rt
//
//  Created by Damir Sagidullin on 21.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

#include "../common/thread.h"
#include <atomic>

namespace mr
{

class IImage;
class ILight;
class IMaterialLayer;
struct MaterialContext;

class SoftwareRenderer
{
	BVH &	m_scene;
	ColorF	m_bgColor;
	Vec3	m_envColor;
	const IImage *m_pEnvironmentMap;
	bool	m_showFloor;
	float	m_floorShadow;
	float	m_floorIOR;
	float	m_ambientOcclusion;
	float	m_focalDistance;
	float	m_dofBlur;
	int		m_numAmbientOcclusionSamples;
	std::vector<ILight *>	m_lights;

	IImage *m_pImage;
	RectI	m_rcRenderArea;
	Vec3	m_vEyePos;
	float	m_fFrameBlend;
	Vec3	m_vCamDelta[3];
	Vec2	m_dp;
	Vec2	m_dofLC;
	Vec2	m_dofDP;

	std::atomic<int>	m_nAreaCounter;
	int		m_numAreasX;
	int		m_numAreas;
	typedef TVec2<int>	Vec2I;
	Vec2I	m_delta;
	Vec2	m_random;
	Matrix	m_matRandom;

	float	m_fDistEpsilon;
	float	m_fRayLength;
	int		m_nMaxDepth;

	struct MaterialStack : public ITriangleChecker
	{
		enum {MAX_STACK_DEPTH = 64};
		int nCount;
		const IMaterialLayer *pMaterials[MAX_STACK_DEPTH];

		MaterialStack() : nCount(0) {}
		MaterialStack(const MaterialStack & mc);

		int FindMaterial(const IMaterialLayer *pMaterial) const;
		void Add(const IMaterialLayer *pMaterial) { pMaterials[nCount++] = pMaterial; }
		void Remove(int i);

		bool CheckTriangle(const CollisionTriangle *pTriangle, bool backface) const;
	};

	std::vector<Thread *> m_renderThreads;

	bool GetNextArea(RectI & rc);
	void RenderArea(const RectI & rc) const;

	struct Result
	{
		Vec3 color;
		Vec3 opacity;
		Vec3 pos;
		Result(const Vec3 & c, const Vec3 & o, const Vec3 & p) : color(c), opacity(o), pos(p) {}
		Result(const ColorF & c, const Vec3 & p) : color(c.r, c.g, c.b), opacity(c.a), pos(p) {}
	};

	Vec3 RandomDirection(const Vec3 & normal) const;
	Vec3 EnvironmentColor(const Vec3 & v) const;
	Result TraceRay(const Vec3 & v1, const Vec3 & v2, int nTraceDepth, const CollisionTriangle * pPrevTriangle, MaterialStack & ms) const;
	ColorF RenderPixel(const Vec2 & p) const;

	inline void AddAmbientOcclusion(Vec3 & color, const Vec3 & P, const Vec3 & N, const Vec3 & TN, int numSamples, const TraceResult & tr,
									const IMaterialLayer * pMaterial, const MaterialContext & mc, float bumpZ) const;
	inline void AddLighting(Vec3 & color, const Vec3 & P, const Vec3 & N, const TraceResult & tr,
							const IMaterialLayer * pMaterial, const MaterialContext & mc, float bumpZ) const;
	inline Vec3 CalcFloorIllumination(const Vec3 & P) const;

	static void ThreadFunc(void * pRenderer);

public:

	SoftwareRenderer(BVH & scene);
	~SoftwareRenderer();

	size_t RaysCounter() const { return m_scene.RaysCounter(); }
	void ResetRayCounter() { m_scene.ResetRayCounter(); }

	void SetBackgroundColor(const ColorF & bgColor);
	void SetEnvironmentColor(const ColorF & envColor);
	void SetEnvironmentMap(const IImage * pEnvironmentMap);
	void SetShowFloor(bool b) { m_showFloor = b; }
	void SetFloorIOR(float ior) { m_floorIOR = ior; }
	void SetFloorShadow(float f) { m_floorShadow = f; }
	void SetAmbientOcclusion(float f, size_t numSamples);
	void SetDepthOfField(float blur);
	void SetFocalDistance(float dist);
	void SetLights(size_t num, ILight ** ppLights);

	void Render(IImage & image, const RectI * pViewportRect,
				const Matrix & matCamera, const Matrix & matViewProj, const Vec2 & vPixelOffset,
				int numThreads, int nFrameNumber);

	void Join();
	void Interrupt();
};

}
