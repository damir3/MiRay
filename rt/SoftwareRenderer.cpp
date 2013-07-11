//
//  SoftwareRenderer.cpp
//  MiRay/rt
//
//  Created by Damir Sagidullin on 21.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#include "SoftwareRenderer.h"
#include "Material.h"

using namespace mr;

SoftwareRenderer::~SoftwareRenderer()
{
	Interrupt();

	while (!m_renderThreads.empty())
	{
		delete m_renderThreads.back();
		m_renderThreads.pop_back();
	}
}

void SoftwareRenderer::Render(Image & image, const RectI * pViewportRect,
							  const Matrix & matCamera, const Matrix & matViewProj,
							  const ColorF & bgColor, const Image * pEnvironmentMap,
							  int numThreads, int nFrameNumber)
{
	m_pImage = &image;
	m_rcRenderArea = pViewportRect ? *pViewportRect : RectI(0, 0, image.Width(), image.Height());
	m_vEyePos = matCamera.Pos();
	m_bgColor = bgColor;
	m_fFrameBlend = 1.f / (nFrameNumber + 1);
	m_pEnvironmentMap = pEnvironmentMap;

	m_fDistEpsilon = m_scene.Root()->BoundingBox().Size().Length() * 0.0001f;
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

	Vec2 vPixelOffset = nFrameNumber > 0 ? Vec2(frand(), frand()) : Vec2(0.5f, 0.5f);
	m_vCamDelta[2] += m_vCamDelta[0] * (vPixelOffset.y * m_dp.x);
	m_vCamDelta[2] += m_vCamDelta[1] * -(vPixelOffset.y * m_dp.y);

	m_pos = Vec2I(0, 0);
	m_delta = Vec2I(16, 16);

	for (size_t i = numThreads; i < m_renderThreads.size(); i++)
		delete m_renderThreads[i];

	while (m_renderThreads.size() < (size_t)numThreads)
		m_renderThreads.push_back(new RenderThread(*this));

	for (size_t i = 0; i < m_renderThreads.size(); i++)
		m_renderThreads[i]->Start();

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
		m_renderThreads[i]->Join();
}

void SoftwareRenderer::Interrupt()
{
	m_mutex.Lock();
	m_pos = Vec2((float)m_rcRenderArea.right, (float)m_rcRenderArea.bottom);
	m_mutex.Unlock();

	Join();
}

bool SoftwareRenderer::GetNextArea(RectI & rc)
{
	Mutex::Locker locker(m_mutex);
	if (m_pos.y >= m_rcRenderArea.bottom)
		return false;

	rc = RectI(m_pos.x, m_pos.y, m_pos.x + m_delta.x, m_pos.y + m_delta.y);
	rc.bottom = std::min<int>(rc.bottom, m_rcRenderArea.bottom);
	if (rc.right >= m_rcRenderArea.right)
	{
		rc.right = m_rcRenderArea.right;

		m_pos.x = m_rcRenderArea.left;
		m_pos.y = rc.bottom;
	}
	else
		m_pos.x = rc.right;

	return true;
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

void SoftwareRenderer::RenderThread::ThreadProc()
{
//	printf("start %p\n", this);

	RectI rc;
	while (m_renderer.GetNextArea(rc))
	{
//		printf("%p: (%d, %d)\n", this, rc.left, rc.top);
		m_renderer.RenderArea(rc);
	}

//	printf("end %p\n", this);
}

ColorF SoftwareRenderer::EnvironmentColor(const Vec3 & vDir) const
{
	if (!m_pEnvironmentMap)
		return m_bgColor;

	return m_pEnvironmentMap->GetPixel(vDir.Yaw() * (1.f / M_2PIf) + 0.5f, vDir.Pitch() * (-1.f / M_PIf) + 0.5f);

//	float l = Vec2(vDir.x, vDir.z).Length();
//	float d = (atan2f(vDir.y, l) * (0.5f / M_PIf) + 0.25f);
//	if (l > 0.f)
//		d /= l;
//
//	return m_pEnvironmentMap->GetPixel(vDir.x * d + 0.5f, vDir.z * -d + 0.5f);
}

float FresnelReflection(const Vec3 & I,const Vec3 & N, float eta)
{
	const float	e = 1.f / eta;
	const float	c = -Vec3::Dot(I, N);
	const float	t = e * e + c * c - 1.f;
	const float	g = sqrtf(std::max<float>(t, 0.f));
	const float	a = (g - c) / (g + c);
	const float	b = (c * (g + c) - 1.f) / (c * (g - c) + 1.f);

	return clamp(0.5f * a * a * (1.f + b * b), 0.f, 1.f);

//	float R0 = powf((eta - 1.f) / (eta + 1.f), 2.f);
//	float ca = Vec3::Dot(I, N);
//	return lerp(powf(1.f + ca, 5.f), 1.f, R0);
}

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

Vec3 FaceForward(const Vec3 & v, const Vec3 & normal)
{
	return Vec3::Dot(v, normal) < 0.f ? -v : v;
}

ColorF SoftwareRenderer::TraceRay(const Vec3 & v1, const Vec3 & v2, int nTraceDepth, const CollisionTriangle * pPrevTriangle) const
{
	TraceResult tr;
	tr.pTriangle = pPrevTriangle;
	if (!m_scene.TraceRay(v1, v2, tr))
		return EnvironmentColor(v2 - v1);

	const MaterialLayer * pMaterial = tr.pTriangle->Material()->Layer(0);
	Vec2 tc = tr.pTriangle->GetTexCoord(tr.pc);
	Vec3 normal = tr.pTriangle->GetNormal(tr.pc);

	if (pMaterial->Normalmap().HasTexture())
	{
		float bumpLevel = pMaterial->BumpLevel().Value(tc);
		if (bumpLevel > 0.f)
		{
			ColorF cn = pMaterial->Normalmap().Color(tc);
			Vec3 tangent, binormal;
			tr.pTriangle->GetTangents(tangent, binormal, normal);
			Vec3 n(cn.r * 2.f - 1.f, cn.g * 2.f - 1.f, cn.b * 2.f - 1.f);
			n = Vec3::Lerp(Vec3::Z, n, bumpLevel);
			normal = tangent * n.x + binormal * n.y + normal * n.z;
			normal.Normalize();
		}
	}

	bool bInverseNormal = Vec3::Dot(tr.pTriangle->Edge1(), Vec3::Cross(v2 - v1, tr.pTriangle->Edge2())) > 0.f;
//	bool bInverseNormal = Vec3::Dot(normal, v1 - v2) < 0.f;
	Vec3 N = bInverseNormal ? -normal : normal;
//	printf("%d: (%g %g %g) -> (%g %g %g) %p (%g %g %g)\n", nTraceDepth, v1.x, v1.y, v1.z, tr.pos.x, tr.pos.y, tr.pos.z, tr.pTriangle, normal.x, normal.y, normal.z);

//	ColorF c = LookUpTexture(p, dp, tr.pTriangle);
	
	ColorF c = pMaterial->Diffuse().Color(tc);
	c.a = pMaterial->Opacity().Value(tc);

	if (c.a > 0.f)
	{
//		ColorF cEnv = ColorF::Null;
//		Vec3 P = tr.pos + N * m_fDistEpsilon;
//		float l = 0.f;
//
//		for (int i = 0; i < 1; i++)
//		{
//			Vec3 vRandDir;
//			float dl;
//			do {
//				vRandDir = Vec3::Normalize(Vec3Rand());
//				dl = Vec3::Dot(vRandDir, N);
//			} while (dl > -0.05f && dl < 0.05f);
//
//			if (dl < 0.f)
//			{
//				dl = -dl;
//				vRandDir = -vRandDir;
//			}
//
//			TraceResult tr2;
//			if (!m_scene.TraceRay(P, P + vRandDir * 200.f, tr2))
//			{
//				cEnv += EnvironmentColor(vRandDir) * dl;
//	//			cEnv += ColorF::White * dl;
//			}
//
//			l += dl;
//		}
//
//		cEnv *= (1.f / l);
//		c.r *= cEnv.r;
//		c.g *= cEnv.g;
//		c.b *= cEnv.b;

		float l = VertexLight(N);
		c.r *= l;
		c.g *= l;
		c.b *= l;
	}

	ColorF reflectionColor = pMaterial->Reflection().Color(tc);
	if (c.a >= 1.f && reflectionColor.r <= 0.f && !pMaterial->FresnelReflection())
		return c; // no reflection, no refraction

	nTraceDepth++;

	Vec3 I = v2 - v1;
	float dist = I.Normalize();

	float ior = pMaterial->IndexOfRefraction().Value(tc);
	float eta = bInverseNormal ? ior : 1.f / ior;
	float kR = reflectionColor.r;
	if (pMaterial->FresnelReflection() && !bInverseNormal)
	{
		float fresnel = bInverseNormal ? FresnelReflection(I, N, ior, 1.f) : FresnelReflection(I, N, 1.f, ior);
		kR = std::min<float>(kR + fresnel, 1.f);
	}
//		kR = std::min<float>(kR + FresnelReflection(I, N, eta), 1.f);

//	bool bReflect = (rand() & 0xFF) * (kR + (1.f - kR) * c.a) <= kR * 0xFF;

	if (c.a < 1.f && kR < 1.f)// && !bReflect)
	{// refraction
		if (nTraceDepth < m_nMaxDepth)
		{
			float refractionGlossiness = pMaterial->RefractionGlossiness().Value(tc);
			Vec3 RN = refractionGlossiness < 1.f ? Vec3::Normalize(N + Vec3Rand() * ((1.f - refractionGlossiness) * 0.25f)) : N;
			Vec3 T = Vec3::Refract(I, RN, eta);
			if (T.Length2() > 0.f)
			{
				Vec3 v1T = tr.pos - N * m_fDistEpsilon;
				ColorF cT = TraceRay(v1T, v1T + T * dist, nTraceDepth, tr.pTriangle);
				cT *= pMaterial->ReflectionTint().Color(tc);

				c = ColorF::LerpRGB(cT, c, c.a, c.a);
			}
		}
		else
			c = ColorF::LerpRGB(pMaterial->RefractionExitColor().Color(tc), c, c.a, c.a);
	}

	if (kR > 0.f)// && bReflect)
	{// reflection
		c.a = std::min<float>(c.a + kR, 1.f); // opacity
		if (nTraceDepth < m_nMaxDepth)
		{
			float reflectionGlossiness = pMaterial->ReflectionGlossiness().Value(tc);
			Vec3 RN = reflectionGlossiness < 1.f ? Vec3::Normalize(N + Vec3Rand() * ((1.f - reflectionGlossiness) * 0.25f)) : N;
			Vec3 R = Vec3::Reflect(I, RN);
			Vec3 v1R = tr.pos + N * m_fDistEpsilon;
			ColorF cR = TraceRay(v1R, v1R + R * dist, nTraceDepth, tr.pTriangle);
			cR *= pMaterial->ReflectionTint().Color(tc);
			c = ColorF::LerpRGB(c, cR, kR, c.a);
		}
		else
			c = ColorF::LerpRGB(c, pMaterial->RefractionExitColor().Color(tc), kR, c.a);
	}

	return c;
}

ColorF SoftwareRenderer::RenderPixel(const Vec2 &p) const
{
	return TraceRay(m_vEyePos, m_vCamDelta[2] + m_vCamDelta[0] * p.x - m_vCamDelta[1] * p.y, 0, NULL);
}

//ColorF SoftwareRenderer::LookUpTexture(const Vec2 & p, const Vec2 & dp, const CollisionTriangle * pTriangle) const
//{
//	const Material * pMaterial = static_cast<const Material *>(pTriangle->UserData());
//	const int nsx = 4;
//	const int nsy = 4;
//	const Vec2 ds(dp.x / nsx, dp.y / nsy);
//	const Vec2 sp(p.x - (nsx - 1) * 0.5f * ds.x, p.y - (nsy - 1) * 0.5f * ds.y);
//	const float d1 = Vec3::Dot(m_vEyePos, pTriangle->Normal()) - pTriangle->Dist();
//	
//	ColorF res = ColorF::Null;
//	for (int sy = 0; sy < nsy; sy++)
//	{
//		for (int sx = 0; sx < nsx; sx++)
//		{
//			Vec4 v(sp.x + sx * ds.x, -(sp.y + sy * ds.y), 1.f, 1.f);
//			v.Transform(matViewProjInv);
//			Vec3 v2 = Vec3(v.x, v.y, v.z) / v.w;
//			
//			float d2 = Vec3::Dot(v2, pTriangle->Normal()) - pTriangle->Dist();
//			if (!((d1 >= 0.f) ^ (d2 >= 0.f)))
//				continue;
//			
//			float dd = d1 - d2;
//			assert(dd != 0.f);
//			float f = d1 / dd;
//			assert(f >= 0.f && f <= 1.f);
//			Vec3 pos = Vec3::Lerp(m_vEyePos, v2, f);
//			
//			Vec2 pc;
//			pTriangle->GetParametricCoordsNoCheck(pc, pos);
//			
//			Vec2 tc = pTriangle->GetTexCoord(pc);
//			res += pMaterial->m_pDiffuseTexture ? pMaterial->m_pDiffuseTexture->GetPixel(tc.x, tc.y) : ColorF::White;
////			res += m_pTexture ? m_pTexture->GetPixel(tc.x, tc.y) : ColorF::White;
//		}
//	}
//	
//	return res * (1.f / (nsx * nsy));
//}
