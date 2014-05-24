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

static const float MIN_NORMALS_DP = cosf(DEG2RAD(85.f));
enum
{
	BUMP_LINEAR_SEARCH_STEPS = 32,
	BUMP_BINARY_SEARCH_STEPS = 5,
	LIGHTING_BUMP_LINEAR_SEARCH_STEPS = 24,
	BUMP_AMBIENT_OCCLUSION_LINEAR_SEARCH_STEPS = 16,
};

// ------------------------------------------------------------------------ //

SoftwareRenderer::SoftwareRenderer(BVH & scene)
	: m_scene(scene)
	, m_pImage(NULL)
	, m_bgColor(ColorF::Null)
	, m_envColor(1.f)
	, m_pEnvironmentMap(NULL)
	, m_ambientOcclusion(1.f)
	, m_focalDistance(0.f)
	, m_dofBlur(0.f)
	, m_numAmbientOcclusionSamples(1)
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
	m_envColor = Vec3(envColor);
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

void SoftwareRenderer::SetFocalDistance(float dist)
{
	printf("dist=%f\n", dist);
	m_focalDistance = dist;
}

void SoftwareRenderer::SetDepthOfField(float blur)
{
	m_dofBlur = blur;
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

//	m_fRayLength = m_scene.BoundingBox().Size().Length();
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

	m_fRayLength = (m_vCamDelta[2] - m_vEyePos).Length();

	m_vCamDelta[2] += m_vCamDelta[0] * (vPixelOffset.y * m_dp.x);
	m_vCamDelta[2] += m_vCamDelta[1] * -(vPixelOffset.y * m_dp.y);

	m_dofLC = Vec2(m_focalDistance / m_fRayLength, m_fRayLength / (m_fRayLength - m_focalDistance));
	m_dofDP = nFrameNumber > 0 ? m_dp * Vec2Rand() * m_dofBlur : Vec2::Null;

	m_delta = Vec2I(16, 16);
	m_nAreaCounter = 0;
	m_numAreasX = (m_rcRenderArea.Width() + m_delta.x - 1) / m_delta.x;
	int numAreasY = (m_rcRenderArea.Height() + m_delta.y - 1) / m_delta.y;
	m_numAreas = m_numAreasX * numAreasY;
	m_random = Vec2(frand(), frand());
	m_matRandom.RotationAxis(Vec3::Normalize(Vec3Rand()), acosf(frand()));

//	numThreads = 1;
	for (int i = 0; i < numThreads; i++)
		m_renderThreads.push_back(new Thread(&ThreadFunc, this));
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
	Vec3 vStart = m_vEyePos;
	Vec3 vDest = m_vCamDelta[2] + m_vCamDelta[0] * p.x - m_vCamDelta[1] * p.y;
	if (m_dofBlur > 0.f)
	{
		Vec3 pos = Vec3::Lerp(m_vEyePos, vDest, m_dofLC.x);
		Vec2 bp = p + m_dofDP;
		vDest = m_vCamDelta[2] + m_vCamDelta[0] * bp.x - m_vCamDelta[1] * bp.y;
		vStart = Vec3::Lerp(vDest, pos, m_dofLC.y);
	}
	MaterialStack ms;
	Result res = TraceRay(vStart, vDest, 0, NULL, ms);
//	printf("\n");
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

Vec3 SoftwareRenderer::EnvironmentColor(const Vec3 & vDir) const
{
	if (!m_pEnvironmentMap)
		return m_envColor;

	return m_pEnvironmentMap->GetPixelColor(vDir.Yaw() * (1.f / M_2PIf) + 0.5f, vDir.Pitch() * (-1.f / M_PIf) + 0.5f) * m_envColor;

//	float l = Vec2(vDir.x, vDir.z).Length();
//	float d = (atan2f(vDir.y, l) * (0.5f / M_PIf) + 0.25f);
//	if (l > 0.f)
//		d /= l;
//
//	return m_pEnvironmentMap->GetPixel(vDir.x * d + 0.5f, vDir.z * -d + 0.5f);
}

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

SoftwareRenderer::MaterialStack::MaterialStack(const MaterialStack & mc) : nCount(mc.nCount)
{
	std::copy(mc.pMaterials, mc.pMaterials + nCount, pMaterials);
}

int SoftwareRenderer::MaterialStack::FindMaterial(const IMaterialLayer *pMaterial) const
{
	int i = nCount;
	while (i-- > 0)
	{
		if (pMaterials[i]->IndexOfRefraction() == pMaterial->IndexOfRefraction())
			return i;
	}

	return -1;
}

void SoftwareRenderer::MaterialStack::Remove(int i)
{
	if (i >= 0)
	{
		nCount--;
		for (int j = i + 1; i < nCount; i = j++)
			pMaterials[i] = pMaterials[j];
	}
}

bool SoftwareRenderer::MaterialStack::CheckTriangle(const CollisionTriangle *pTriangle, bool backface) const
{
	return (FindMaterial(pTriangle->Material()->Layer(0)) < 0) ^ backface;
}

// ------------------------------------------------------------------------ //

Vec2 TraceBumpMap(Vec2 & tc, const Vec3 & dir,
				  const IMaterialLayer * pMaterial, const MaterialContext & mc,
				  int numLinearSearchSteps, int numBinarySearchSteps)
{
	Vec3 dirTS(Vec3::Dot(dir, mc.tangent), Vec3::Dot(dir, mc.binormal), Vec3::Dot(dir, mc.normal));
	float dpDN = dirTS.z;
	dirTS.Normalize();
	dirTS /= fabsf(dirTS.z);
	Vec3 step = dirTS * (1.f / numLinearSearchSteps);

	bool backface = dpDN > 0.f;
	Vec3 pos = backface ? Vec3(tc.x - dirTS.x, tc.y - dirTS.y, 0.f) : Vec3(tc.x, tc.y, 1.f);

	for (int i = 0; i < numLinearSearchSteps; i++)
	{
		if ((pos.z < pMaterial->BumpMapDepth(pos)) ^ backface)
			break;
		
		pos += step;
	}

	for (int i = 0; i < numBinarySearchSteps; i++)
	{
		step *= 0.5f;
		pos -= step;

		if ((pos.z > pMaterial->BumpMapDepth(pos)) ^ backface)
			pos += step;
	}

	tc = pos;

	return Vec2(pos.z, dpDN);
}

// ------------------------------------------------------------------------ //

inline void SoftwareRenderer::AddAmbientOcclusion(Vec3 & color, const Vec3 & P, const Vec3 & N, const Vec3 & TN, int numSamples, const TraceResult & tr,
												  const IMaterialLayer * pMaterial, const MaterialContext & mc, float bumpZ) const
{
	Vec3 ambientOcclusion = Vec3::Null;
	for (int i = 0; i < numSamples; i++)
	{// ambient occlusion
		Vec3 vRandDir;
		do {
			vRandDir = RandomDirection(N);
		} while (Vec3::Dot(vRandDir, TN) <= 0.f);
		
		if (pMaterial->HasBumpMap())
		{
			Vec3 dir = vRandDir.TransformedNormal(tr.pVolume->InverseTransformation());
			Vec2 tc = tr.pTriangle->GetTexCoord(tr.localPos, dir);

			const float Z_EPSILON = 1.f / BUMP_AMBIENT_OCCLUSION_LINEAR_SEARCH_STEPS - 1.f / BUMP_LINEAR_SEARCH_STEPS;
			Vec2 res = TraceBumpMap(tc, -dir, pMaterial, mc, BUMP_AMBIENT_OCCLUSION_LINEAR_SEARCH_STEPS, 0) - Z_EPSILON;
			if ((res.y < 0.f) ? (res.x > bumpZ) : (-res.x > bumpZ))
				continue;
		}
		
		TraceResult otl;
		if (!m_scene.TraceRay(P, P + vRandDir * m_fRayLength, otl))
			ambientOcclusion += EnvironmentColor(vRandDir);
	}

	color += ambientOcclusion * (m_ambientOcclusion / numSamples);
//	color += m_envColor * (N.z * 0.3f + 0.7f);

}

inline void SoftwareRenderer::AddLighting(Vec3 & color, const Vec3 & P, const Vec3 & N, const TraceResult & tr,
										  const IMaterialLayer * pMaterial, const MaterialContext & mc, float bumpZ) const
{
	for (std::vector<ILight *>::const_iterator it = m_lights.begin(); it != m_lights.end(); ++it)
	{// lighting
		const ILight * pLight = *it;
		Vec3 lightPos = pLight->Position(P);
		Vec3 lightDir = lightPos - P;
		float l2 = lightDir.LengthSquared();
		if (l2 == 0.f)
			continue;

		float dp = Vec3::Dot(N, lightDir);
		if (dp <= 0.f)
			continue;

		Vec3 vLightIntensity = pLight->Intensity(l2);
		if (vLightIntensity == Vec3::Null)
			continue;

		if (pMaterial->HasBumpMap())
		{
			Vec3 dir = tr.localPos - lightPos.TransformedCoord(tr.pVolume->InverseTransformation());
			Vec2 tc = tr.pTriangle->GetTexCoord(tr.localPos, dir);

			const float Z_EPSILON = 1.f / LIGHTING_BUMP_LINEAR_SEARCH_STEPS - 1.f / BUMP_LINEAR_SEARCH_STEPS;
			Vec2 res = TraceBumpMap(tc, dir, pMaterial, mc, LIGHTING_BUMP_LINEAR_SEARCH_STEPS, 0) - Z_EPSILON;
			if ((res.y < 0.f) ? (res.x > bumpZ) : (-res.x > bumpZ))
				continue;
		}

		TraceResult ltr;
		if (m_scene.TraceRay(P, lightPos, ltr))
			continue;

		// diffuse
		color += vLightIntensity * (dp / sqrtf(l2));

//		// specular
//		if (bReflection)
//			cReflection += pLight->Intensity(P, P + R * dist);
	}
}

inline void FixNormal(Vec3 &normal, const Vec3 &triangleNormal)
{
	float dp = Vec3::Dot(normal, triangleNormal);
	if (dp <= MIN_NORMALS_DP)
	{
		normal -= (dp - MIN_NORMALS_DP)  * triangleNormal;
		normal.Normalize();
	}
}

// ------------------------------------------------------------------------ //

SoftwareRenderer::Result SoftwareRenderer::TraceRay(const Vec3 & v1, const Vec3 & v2, int nTraceDepth, const CollisionTriangle * pPrevTriangle, MaterialStack & ms) const
{
	TraceResult tr;
	tr.pTriangle = pPrevTriangle;
	tr.pTC = &ms;

	int nMaterialIndex;
	Vec3 vOrigin = v1;
	Vec3 I = v2 - v1;
	while (true)
	{
		if (!m_scene.TraceRay(vOrigin, v2, tr))
			return Result(EnvironmentColor(I), nTraceDepth == 0 ? Vec3::Null : Vec3(1.f), v2);

		nMaterialIndex = ms.FindMaterial(tr.pTriangle->Material()->Layer(0));
		if ((nMaterialIndex < 0) ^ tr.backface)
			break;

		vOrigin = tr.pos + I * 1e-6f;
	}

	I.Normalize();

	const IMaterialLayer * pMaterial = tr.pTriangle->Material()->Layer(0);
	MaterialContext mc;
	mc.tc = tr.pTriangle->GetTexCoord(tr.pc);
	mc.dir = tr.localDir;
	mc.normal = Vec3::Normalize(tr.pTriangle->GetNormal(tr.pc)); // local space normal

	Vec3 normal = mc.normal;
	Vec2 bumpRes;

	if (pMaterial->HasBumpMap())
	{// bump mapping
		tr.pTriangle->GetTangents(mc.tangent, mc.binormal, mc.normal, pMaterial->BumpDepth());

		mc.dir.Normalize();

		bumpRes = TraceBumpMap(mc.tc, mc.dir, pMaterial, mc, BUMP_LINEAR_SEARCH_STEPS, BUMP_BINARY_SEARCH_STEPS);
		tr.localPos += mc.dir * (pMaterial->BumpDepth() * (bumpRes.x - 1.f) / bumpRes.y);

		normal = pMaterial->BumpMapNormal(mc);
	}

	Vec3 triangleNormal = tr.pTriangle->Normal();
	if (tr.pVolume)
	{
		normal.TTransformNormal(tr.pVolume->InverseTransformation());
		normal.Normalize();
		triangleNormal.TTransformNormal(tr.pVolume->InverseTransformation());
		triangleNormal.Normalize();
	}
	else
		normal.Normalize();

	FixNormal(normal, triangleNormal);

	Vec3 N = tr.backface ? -normal : normal; // faceforward normal
	Vec3 TN = tr.backface ? -triangleNormal : triangleNormal; // triangle normal
	float dpIN = Vec3::Dot(I, TN);
	/*{
		Vec3 p1 = vOrigin.TransformedCoord(tr.pVolume->InverseTransformation());
		Vec3 p2 = v2.TransformedCoord(tr.pVolume->InverseTransformation());
		Vec3 I2 = p2 - p1;
		I2.Normalize();
		Vec3 TN2 = tr.pTriangle->Normal();
		TN2.Normalize();

		Vec3 TN1 = TN2;
		TN1.TransformNormal(tr.pVolume->Transformation());
		TN1.Normalize();
		
		float dp1 = Vec3::Dot(I, TN1);
		float dp2 = Vec3::Dot(I2, TN2);
		if ((dp1 > 0.f) ^ tr.backface)
		{
			printf("%g %g\n", dp1, dp2);
			TraceResult tr2;
			CollisionRay ray(p1, p2, tr2);
			tr.pTriangle->TraceRay(ray, tr2);
		}
	}*/
	assert(dpIN < 0.f);
//	assert(Vec3::Dot(N, TN) > 0.f);

//	printf("%d: (%g %g %g) -> (%g %g %g) %p (%g %g %g)\n", nTraceDepth, v1.x, v1.y, v1.z, tr.pos.x, tr.pos.y, tr.pos.z, tr.pTriangle, normal.x, normal.y, normal.z);

	Vec3 kR = pMaterial->Reflection(mc);
	
	Vec3 ior = pMaterial->IndexOfRefraction();
	if (pMaterial->FresnelReflection()/* && !tr.backface*/)
	{// update reflectivity
		Vec3 prevIOR(1.f);// = ms.GetIOR();
		float ior1 = 0.f, ior2 = 1.f;
		float fresnel = 0.f;
		for (int i = 0; i < 3; i++)
		{
			if (ior1 != ior[i] || ior2 != prevIOR[i])
			{
				ior1 = ior[i];
				ior2 = prevIOR[i];
				fresnel = tr.backface ? FresnelReflection(I, N, ior1, ior2) : FresnelReflection(I, N, ior2, ior1);
			}
			kR[i] = fminf(kR[i] + fresnel, 1.f);
		}
	}

	bool bReflection = (kR.x > 0.f || kR.y > 0.f || kR.z > 0.f);
	Vec3 R; // reflection direction
	Result cR(Vec3::Null, Vec3::Null, tr.pos);

	nTraceDepth++;
//	if (frand() < kR.x)
	if (bReflection)
	{// reflection
		if (nTraceDepth >= m_nMaxDepth)
		{
			cR = pMaterial->HasReflectionExitColor() ? Result(pMaterial->ReflectionExitColor(mc), Vec3(1.f), tr.pos) :
														Result(EnvironmentColor(I), tr.pos);
		}
		else
		{
			float reflectionRoughness = pMaterial->ReflectionRoughness(mc);
			Vec3 RN = reflectionRoughness > 0.f ? Vec3::Normalize(N + Vec3Rand() * (reflectionRoughness * 0.25f)) : N;
			R = Vec3::Reflect(I, RN);
			float dp = Vec3::Dot(R, TN);
			if (dp < 0.f) R -= TN * dp;
			Vec3 v1R = tr.pos + TN * m_fDistEpsilon;
			MaterialStack msR(ms);
			cR = TraceRay(v1R, v1R + R * m_fRayLength, nTraceDepth, tr.pTriangle, msR);
			cR.color.Scale(pMaterial->ReflectionTint(mc));

			if (tr.backface)
			{
				// absorption (Beer–Lambert law)
				Vec3 absorbtionExp = pMaterial->AbsorbtionCoefficient() * (cR.pos - v1R).Length();
				cR.color.x *= expf(absorbtionExp.x);
				cR.color.y *= expf(absorbtionExp.y);
				cR.color.z *= expf(absorbtionExp.z);
			}

			if ((kR.x >= 1.f && kR.y >= 1.f && kR.z >= 1.f))
			{
				cR.color += pMaterial->Emissive(mc);
				return cR;
			}
		}
	}

	Vec3 opacity = pMaterial->Opacity(mc);
	bool bTransmission = (opacity.x < 1.f || opacity.y < 1.f || opacity.z < 1.f);
	Result cT(Vec3::Null, Vec3::Null, tr.pos);

//	if (frand() > opacity.x)
	if (bTransmission)
	{// transmission
		if (nTraceDepth >= m_nMaxDepth)
		{
			cT = pMaterial->HasRefractionExitColor() ? Result(pMaterial->RefractionExitColor(mc), Vec3(1.f), tr.pos) :
													   Result(EnvironmentColor(I), tr.pos);
		}
		else
		{
			float refractionRoughness = pMaterial->RefractionRoughness(mc);
			Vec3 RN = refractionRoughness > 0.f ? Vec3::Normalize(N + Vec3Rand() * (refractionRoughness * 0.25f)) : N;
			float eta = tr.backface ? ior.x : 1.f / ior.x;
//			float eta;
//			printf("%c %g ", tr.backface ? '<' : '>', ior.x);
//			if (tr.backface)
//			{
//				Vec3 prevIOR = ms.Pop(pMaterial);
//				if (prevIOR.x != 1.f)
//					return Result(Vec3(10.f, 0.f, 0.f), Vec3(1.f), tr.pos);
//
//				eta = ior.x / prevIOR.x;
//			}
//			else
//			{
//				Vec3 prevIOR = ms.Push(pMaterial);
//				if (prevIOR.x != 1.f)
//					return Result(Vec3(0.f, 10.f, 0.f), Vec3(1.f), tr.pos);
//				eta = prevIOR.x / ior.x;
//			}
//			eta = eta1;
			
			Vec3 T = Vec3::Refract(I, RN, eta);
			float dp = Vec3::Dot(T, TN);
			if (dp > 0.f) T -= TN * dp;
			if (T.Normalize() == 0.f)
			{
				cT = pMaterial->HasRefractionExitColor() ? Result(pMaterial->RefractionExitColor(mc), Vec3(1.f), tr.pos) :
															Result(EnvironmentColor(I), tr.pos);
			}
			else
			{
				if (tr.backface)
					ms.Remove(nMaterialIndex);
				else
					ms.Add(pMaterial);

				Vec3 v1T = tr.pos - TN * m_fDistEpsilon;
				cT = TraceRay(v1T, v1T + T * m_fRayLength, nTraceDepth, tr.pTriangle, ms);
				if (!tr.backface)
				{
					// absorption (Beer–Lambert law)
					Vec3 absorbtionExp = pMaterial->AbsorbtionCoefficient() * (cT.pos - v1T).Length();
					cT.color.x *= expf(absorbtionExp.x);
					cT.color.y *= expf(absorbtionExp.y);
					cT.color.z *= expf(absorbtionExp.z);
					
					cT.color.Scale(pMaterial->RefractionTint(mc));
				}
			}

//			return cT;
		}
	}

	Vec3 P = tr.pos + triangleNormal * m_fDistEpsilon;
	Result res(Vec3::Null, opacity, tr.pos);

	if (opacity.x > 0.f || opacity.y > 0.f || opacity.z > 0.f)
	{
		res.color += pMaterial->Ambient(mc);

		if (m_ambientOcclusion > 0.f)
		{
			float maxOpacity = fmaxf(fmaxf(opacity.x, opacity.y), opacity.z);
			int numSamples = std::max<int>((int)(maxOpacity * m_ambientOcclusion * m_numAmbientOcclusionSamples), 1);
			AddAmbientOcclusion(res.color, P, normal, triangleNormal, numSamples, tr, pMaterial, mc, bumpRes.x);
		}

		AddLighting(res.color, P, normal, tr, pMaterial, mc, bumpRes.x);

		res.color.Scale(pMaterial->Diffuse(mc));
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
