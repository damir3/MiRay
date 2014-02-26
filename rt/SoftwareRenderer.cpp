//
//  SoftwareRenderer.cpp
//  MiRay/rt
//
//  Created by Damir Sagidullin on 21.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#include "SoftwareRenderer.h"
#include "Material.h"
#include "Light.h"
#include "Image.h"

using namespace mr;

// ------------------------------------------------------------------------ //

SoftwareRenderer::SoftwareRenderer(BVH & scene)
	: m_scene(scene)
	, m_pImage(NULL)
	, m_bgColor(ColorF::Null)
	, m_envColor(ColorF::White)
	, m_pEnvironmentMap(NULL)
	, m_ambientOcclusion(1.f)
	, m_numAmbientOcclusionSamples(1)
	, m_nRayCounter(0)
{
}

SoftwareRenderer::~SoftwareRenderer()
{
	Interrupt();
	Join();
}

// ------------------------------------------------------------------------ //

void SoftwareRenderer::SetBackgroundColor(const ColorF & bgColor)
{
	m_bgColor = bgColor;
}

void SoftwareRenderer::SetEnvironmentColor(const ColorF & envColor)
{
	m_envColor = envColor;
}

void SoftwareRenderer::SetEnvironmentMap(const IImage * pEnvironmentMap)
{
	m_pEnvironmentMap = pEnvironmentMap;
}

void SoftwareRenderer::SetAmbientOcclusion(float f, size_t numSamples)
{
	m_ambientOcclusion = f;
	m_numAmbientOcclusionSamples = numSamples;
}

void SoftwareRenderer::SetLights(size_t num, ILight ** ppLights)
{
	m_lights.resize(num);
	for (size_t i = 0; i < num; i++)
		m_lights[i] = ppLights[i];
}

// ------------------------------------------------------------------------ //

void SoftwareRenderer::Render(IImage & image, const RectI * pViewportRect,
							  const Matrix & matCamera, const Matrix & matViewProj, const Vec2 & vPixelOffset,
							  int numThreads, int nFrameNumber)
{
	m_pImage = &image;
	m_rcRenderArea = pViewportRect ? *pViewportRect : RectI(0, 0, image.Width(), image.Height());
	m_vEyePos = matCamera.Pos();
	m_fFrameBlend = 1.f / (nFrameNumber + 1);

	m_fRayLength = m_scene.BoundingBox().Size().Length();
	m_fDistEpsilon = m_fRayLength * 0.0001f;
	m_nMaxDepth = 6;
	m_dp = Vec2(2.f / m_rcRenderArea.Width(), 2.f / m_rcRenderArea.Height());

	Matrix matViewProjInv;
	matViewProjInv.Inverse(matViewProj);
	Vec4 p = Vec4(0.f, 0.f, 1.f, 1.f);
	p.Transform(matViewProjInv);
	m_vCamDelta[2] = Vec3(p.x, p.y, p.z) / p.w;
	p = Vec4(1.f, 0.f, 1.f, 1.f);
	p.Transform(matViewProjInv);
	m_vCamDelta[0] = Vec3(p.x, p.y, p.z) / p.w - m_vCamDelta[2];
	p = Vec4(0.f, 1.f, 1.f, 1.f);
	p.Transform(matViewProjInv);
	m_vCamDelta[1] = Vec3(p.x, p.y, p.z) / p.w - m_vCamDelta[2];

	m_vCamDelta[2] += m_vCamDelta[0] * (vPixelOffset.y * m_dp.x);
	m_vCamDelta[2] += m_vCamDelta[1] * -(vPixelOffset.y * m_dp.y);

	m_delta = Vec2I(16, 16);
	m_nAreaCounter = 0;
	m_numAreasX = (m_rcRenderArea.Width() + m_delta.x - 1) / m_delta.x;
	int numAreasY = (m_rcRenderArea.Height() + m_delta.y - 1) / m_delta.y;
	m_numAreas = m_numAreasX * numAreasY;
	m_random = Vec2(frand(), frand());
	m_matRandom.RotationAxis(Vec3::Normalize(Vec3Rand()), acosf(frand()));

	for (int i = 0; i < numThreads; i++)
		m_renderThreads.push_back(new Thread(&ThreadFunc, this));

//	printf("x1\n");

//	RectI rc;
//	while (GetNextArea(rc))
//		RenderArea(rc);

//	printf("x2\n");

//	ColorF cScale(0.5f, 0.5f, 0.5f, 1.f);
//	for (int y = 0; y < image.Height(); y++)
//	{
//		for (int x = 0; x < image.Width(); x++)
//		{
//			ColorF c = image.GetPixel(x, y) * cScale;
//			image.SetPixel(x, y, c);
//		}
//	}
//	printf("x3\n");
}

void SoftwareRenderer::Join()
{
	for (size_t i = 0; i < m_renderThreads.size(); i++)
	{
		m_renderThreads[i]->join();
		delete m_renderThreads[i];
	}

	m_renderThreads.clear();
}

void SoftwareRenderer::Interrupt()
{
	m_nAreaCounter = m_numAreas;
}

// ------------------------------------------------------------------------ //

bool SoftwareRenderer::GetNextArea(RectI & rc)
{
	int pos = m_nAreaCounter++;
	if (pos >= m_numAreas)
		return false;

	rc.left = m_rcRenderArea.left + m_delta.x * (pos % m_numAreasX);
	rc.top = m_rcRenderArea.top + m_delta.y * (pos / m_numAreasX);
	rc.right = std::min(rc.left + m_delta.x, m_rcRenderArea.right);
	rc.bottom = std::min(rc.top + m_delta.y, m_rcRenderArea.bottom);
	return true;
}

ColorF SoftwareRenderer::RenderPixel(const Vec2 &p) const
{
	sResult res = TraceRay(m_vEyePos, m_vCamDelta[2] + m_vCamDelta[0] * p.x - m_vCamDelta[1] * p.y, 0, NULL);
	return ColorF(res.color.x, res.color.y, res.color.z, res.opacity.x);
}

void SoftwareRenderer::RenderArea(const RectI & rc) const
{
	Vec2 p;
	for (int y = rc.top; y < rc.bottom; y++)
	{
		p.y = y * m_dp.y - 1.f;
		
		for (int x = rc.left; x < rc.right; x++)
		{
			p.x = x * m_dp.x - 1.f;

			ColorF res = RenderPixel(p);
			if (m_fFrameBlend < 1.f)
			{
				ColorF src = m_pImage->GetPixel(x, y);
				m_pImage->SetPixel(x, y, ColorF::Lerp(src, res, m_fFrameBlend));
			}
			else
				m_pImage->SetPixel(x, y, res);
		}
	}
}

void SoftwareRenderer::ThreadFunc(void * pRenderer)
{
//	printf("start %p\n", this);
	SoftwareRenderer * pThis = reinterpret_cast<SoftwareRenderer *>(pRenderer);

	RectI rc;
	while (pThis->GetNextArea(rc))
	{
//		printf("%p: (%d, %d)\n", this, rc.left, rc.top);
		pThis->RenderArea(rc);
	}

//	printf("end %p\n", this);
}

// ------------------------------------------------------------------------ //

ColorF SoftwareRenderer::EnvironmentColor(const Vec3 & vDir) const
{
	if (!m_pEnvironmentMap)
		return m_envColor;

	return m_pEnvironmentMap->GetPixel(vDir.Yaw() * (1.f / M_2PIf) + 0.5f, vDir.Pitch() * (-1.f / M_PIf) + 0.5f) * m_envColor;

//	float l = Vec2(vDir.x, vDir.z).Length();
//	float d = (atan2f(vDir.y, l) * (0.5f / M_PIf) + 0.25f);
//	if (l > 0.f)
//		d /= l;
//
//	return m_pEnvironmentMap->GetPixel(vDir.x * d + 0.5f, vDir.z * -d + 0.5f);
}

//float FresnelReflection(const Vec3 & I,const Vec3 & N, float eta)
//{
//	const float	e = 1.f / eta;
//	const float	c = -Vec3::Dot(I, N);
//	const float	t = e * e + c * c - 1.f;
//	const float	g = sqrtf(std::max<float>(t, 0.f));
//	const float	a = (g - c) / (g + c);
//	const float	b = (c * (g + c) - 1.f) / (c * (g - c) + 1.f);
//
//	return clamp(0.5f * a * a * (1.f + b * b), 0.f, 1.f);
//
////	float R0 = powf((eta - 1.f) / (eta + 1.f), 2.f);
////	float ca = Vec3::Dot(I, N);
////	return lerp(powf(1.f + ca, 5.f), 1.f, R0);
//}

float FresnelReflection(const Vec3 & I,const Vec3 & N, float n1, float n2)
{
	const float	n = n1 / n2;
	const float cosI = -Vec3::Dot(N, I);
	const float sinT2 = n * n * (1.f - cosI * cosI);
	if (sinT2 > 1.f) return 1.f;
	const float cosT = sqrtf(1.f - sinT2);
	const float rO = (n1 * cosI - n2 * cosT) / (n1 * cosI + n2 * cosT);
	const float rP = (n2 * cosI - n1 * cosT) / (n2 * cosI + n1 * cosT);
	return (rO * rO + rP * rP) * 0.5f;
}

//Vec3 FaceForward(const Vec3 & v, const Vec3 & normal)
//{
//	return Vec3::Dot(v, normal) < 0.f ? -v : v;
//}

SoftwareRenderer::sResult SoftwareRenderer::TraceRay(const Vec3 & v1, const Vec3 & v2, int nTraceDepth, const CollisionTriangle * pPrevTriangle) const
{
	TraceResult tr;
	tr.pTriangle = pPrevTriangle;
	++const_cast<SoftwareRenderer &>(*this).m_nRayCounter;
	if (!m_scene.TraceRay(v1, v2, tr))
		return EnvironmentColor(v2 - v1);

	const IMaterialLayer * pMaterial = tr.pTriangle->Material()->Layer(0);
	sMaterialContext mc;
	Vec2 tc = tr.pTriangle->GetTexCoord(tr.pc);
	Vec3 normal = tr.pTriangle->GetNormal(tr.pc);
	mc.tc = tc;
	mc.normal = normal;
	mc.dir = v2 - v1;

	if (pMaterial->HasNormalMap())
	{// bump mapping
		Vec3 nm = pMaterial->NormalMap(mc);
		if (nm.x != 0.f || nm.y != 0.f)
		{
			Vec3 tangent, binormal;
			tr.pTriangle->GetTangents(tangent, binormal, normal);
			normal = tangent * nm.x + binormal * nm.y + normal * nm.z;
		}
	}

	if (tr.pVolume)
		normal = normal.TransformedNormal(tr.pVolume->Transformation());

	normal.Normalize();
	Vec3 N = tr.backface ? -normal : normal;

//	printf("%d: (%g %g %g) -> (%g %g %g) %p (%g %g %g)\n", nTraceDepth, v1.x, v1.y, v1.z, tr.pos.x, tr.pos.y, tr.pos.z, tr.pTriangle, normal.x, normal.y, normal.z);

	Vec3 kR = pMaterial->Reflection(mc);
	Vec3 I = Vec3::Normalize(v2 - v1);
	
	Vec3 ior = pMaterial->IndexOfRefraction(mc);
	if (pMaterial->FresnelReflection()/* && !tr.backface*/)
	{// update reflectivity
		float ior1 = 0.f, ior2 = 1.f;
		float fresnel = 0.f;
		for (int i = 0; i < 3; i++)
		{
			if (ior1 != ior[i])
			{
				ior1 = ior[i];
				fresnel = tr.backface ? FresnelReflection(I, N, ior1, ior2) : FresnelReflection(I, N, ior2, ior1);
			}
			kR[i] = std::min<float>(kR[i] + fresnel, 1.f);
		}
	}

	bool bReflection = (kR.x > 0.f || kR.y > 0.f || kR.z > 0.f);
	Vec3 R; // reflection direction
	sResult cR(Vec3::Null, Vec3::Null);

	nTraceDepth++;
//	if (frand() < kR.x)
	if (bReflection)
	{// reflection
		if (nTraceDepth >= m_nMaxDepth)
			return sResult(pMaterial->RefractionExitColor(mc), Vec3(0.f));
		
		float reflectionRoughness = pMaterial->ReflectionRoughness(mc);
		Vec3 RN = reflectionRoughness > 0.f ? Vec3::Normalize(N + Vec3Rand() * (reflectionRoughness * 0.25f)) : N;
		R = Vec3::Reflect(I, RN);
		Vec3 v1R = tr.pos + N * m_fDistEpsilon;
		cR = TraceRay(v1R, v1R + R * m_fRayLength, nTraceDepth, tr.pTriangle);
		cR.color.Scale(pMaterial->ReflectionTint(mc));

		if ((kR.x >= 1.f && kR.y >= 1.f && kR.z >= 1.f))
		{
			cR.color += pMaterial->Emissive(mc);
			return cR;
		}
	}

	Vec3 opacity = pMaterial->Opacity(mc);
	bool bTransmission = (opacity.x < 1.f || opacity.y < 1.f || opacity.z < 1.f);
	sResult cT(Vec3::Null, Vec3::Null);

//	if (frand() > opacity.x)
	if (bTransmission)
	{// transmission
		if (nTraceDepth >= m_nMaxDepth)
			return sResult(pMaterial->RefractionExitColor(mc), Vec3(0.f));

		float refractionRoughness = pMaterial->RefractionRoughness(mc);
		Vec3 RN = refractionRoughness > 0.f ? Vec3::Normalize(N + Vec3Rand() * (refractionRoughness * 0.25f)) : N;
		float eta = tr.backface ? ior.x : 1.f / ior.x;
		Vec3 T = Vec3::Refract(I, RN, eta);
		if (T.Normalize() == 0.f)
			return sResult(pMaterial->RefractionExitColor(mc), Vec3(0.f));

		Vec3 v1T = tr.pos - N * m_fDistEpsilon;
		cT = TraceRay(v1T, v1T + T * m_fRayLength, nTraceDepth, tr.pTriangle);
		if (!tr.backface)
			cT.color.Scale(pMaterial->RefractionTint(mc));

//		return cT;
	}

	Vec3 P = tr.pos + normal * m_fDistEpsilon;
	sResult res(Vec3::Null, opacity);

	if (opacity.x > 0.f || opacity.y > 0.f || opacity.z > 0.f)
	{
		Vec3 diffuse = pMaterial->Diffuse(mc);
		res.color += pMaterial->Ambient(mc);

		if (m_ambientOcclusion > 0.f)
		{
			Vec3 ambientOcclusion = Vec3::Null;
			float maxOpacity = std::max<float>(std::max<float>(opacity.x, opacity.y), opacity.z);
			int numSamples = std::max<int>((int)(maxOpacity * m_ambientOcclusion * m_numAmbientOcclusionSamples), 1);
			for (int i = 0; i < numSamples; i++)
			{// ambient occlusion
				Vec3 vRandDir = RandomDirection(normal);
				TraceResult otl;
				++const_cast<SoftwareRenderer &>(*this).m_nRayCounter;
				if (!m_scene.TraceRay(P, P + vRandDir * m_fRayLength, otl))
				{
					ColorF envColor = EnvironmentColor(vRandDir);
					ambientOcclusion += Vec3(envColor.r, envColor.g, envColor.b);
				}
			}
			res.color += ambientOcclusion * (m_ambientOcclusion / numSamples);
		}

		if (!m_lights.empty())
		{// lighting
			ILight * pLight = m_lights[rand() % m_lights.size()];
			Vec3 vLightPos = pLight->Position(P);
			Vec3 vLightDir = vLightPos - P;
			float l2 = vLightDir.LengthSquared();
			if (l2 > 0.f)
			{
				float dp = Vec3::Dot(normal, vLightDir);
				if (dp > 0.f)
				{
					TraceResult ltr;
					++const_cast<SoftwareRenderer &>(*this).m_nRayCounter;
					if (!m_scene.TraceRay(P, vLightPos, ltr))
					{
						// diffuse
						Vec3 vLightIntensity = pLight->Intensity(l2);
						if (vLightIntensity != Vec3::Null)
							res.color += vLightIntensity * dp / sqrtf(l2);
						
//						// specular
//						if (bReflection)
//							cReflection += pLight->Intensity(P, P + R * dist);
					}
				}
			}
		}

		res.color.Scale(diffuse);
	}

	if (bTransmission)
	{
		res.color = Vec3::Lerp3(cT.color, res.color, opacity);
		cT.opacity.Scale(Vec3(1.f) - res.opacity);
		res.opacity += cT.opacity;
	}

	if (bReflection)
	{
		res.color = Vec3::Lerp3(res.color, cR.color, kR);
		res.opacity = Vec3::Lerp3(res.opacity, cR.opacity, kR);
	}

	res.color += pMaterial->Emissive(mc);

	return res;
}

// ------------------------------------------------------------------------ //

Vec3 SoftwareRenderer::RandomDirection(const Vec3 & normal) const
{
	Vec3 axis1 = normal.Perpendicular();
	Vec3 axis2 = Vec3::Cross(normal, axis1);
//	float cosA = m_random.x;
	float cosA = frand();
	float sinA = sqrtf(1.f - cosA * cosA);
//	float B = m_random.y * M_2PIf;
	float B = rand() * (M_2PIf / RAND_MAX);
	return axis1 * (sinA * cosf(B)) + axis2 * (sinA * sinf(B)) + normal * cosA;
//	return normal.TransformedNormal(m_matRandom);
}

// ------------------------------------------------------------------------ //

/*
ColorF SoftwareRenderer::LookUpTexture(const Vec2 & p, const Vec2 & dp, const CollisionTriangle * pTriangle) const
{
	const Material * pMaterial = static_cast<const Material *>(pTriangle->UserData());
	const int nsx = 4;
	const int nsy = 4;
	const Vec2 ds(dp.x / nsx, dp.y / nsy);
	const Vec2 sp(p.x - (nsx - 1) * 0.5f * ds.x, p.y - (nsy - 1) * 0.5f * ds.y);
	const float d1 = Vec3::Dot(m_vEyePos, pTriangle->Normal()) - pTriangle->Dist();
	
	ColorF res = ColorF::Null;
	for (int sy = 0; sy < nsy; sy++)
	{
		for (int sx = 0; sx < nsx; sx++)
		{
			Vec4 v(sp.x + sx * ds.x, -(sp.y + sy * ds.y), 1.f, 1.f);
			v.Transform(matViewProjInv);
			Vec3 v2 = Vec3(v.x, v.y, v.z) / v.w;
			
			float d2 = Vec3::Dot(v2, pTriangle->Normal()) - pTriangle->Dist();
			if (!((d1 >= 0.f) ^ (d2 >= 0.f)))
				continue;
			
			float dd = d1 - d2;
			assert(dd != 0.f);
			float f = d1 / dd;
			assert(f >= 0.f && f <= 1.f);
			Vec3 pos = Vec3::Lerp(m_vEyePos, v2, f);
			
			Vec2 pc;
			pTriangle->GetParametricCoordsNoCheck(pc, pos);
			
			Vec2 tc = pTriangle->GetTexCoord(pc);
			res += pMaterial->m_pDiffuseTexture ? pMaterial->m_pDiffuseTexture->GetPixel(tc.x, tc.y) : ColorF::White;
//			res += m_pTexture ? m_pTexture->GetPixel(tc.x, tc.y) : ColorF::White;
		}
	}
	
	return res * (1.f / (nsx * nsy));
}
*/
