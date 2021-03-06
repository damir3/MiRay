//
//  CollisionTriangle.h
//  MiRay/rt
//
//  Created by Damir Sagidullin on 01.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

#include "CollisionRay.h"

namespace mr
{

class CollisionTriangle;
class CollisionVolume;
class IMaterial;

class ITriangleChecker
{
public:
	virtual bool CheckTriangle(const CollisionTriangle *pTriangle, bool backface) const = 0;
};

struct TraceResult
{
	const CollisionTriangle * pTriangle;
	CollisionVolume * pVolume;
	float	fraction;
	bool	backface;
	Vec3	pos;
	Vec2	pc;
	Vec3	localPos;
	Vec3	localDir;

	ITriangleChecker *pTC;

	TraceResult() : pTriangle(NULL), pVolume(NULL), fraction(1.f), pTC(NULL) {}
};

class CollisionTriangle
{
	Vec3		m_normal;
	int			m_axis_u, m_axis_v;
	float		m_e1u, m_e1v;
	float		m_e2u, m_e2v;
	const IMaterial * m_pMaterial;

	Vertex		m_vertices[3];
	BBox		m_bbox;
	Vec3		m_edgeU;
	Vec3		m_edgeV;

//#ifdef USE_SSE
//	__m128		m_center;
//	__m128		m_extents;
//#else
//	Vec3		m_center;
//	Vec3		m_extents;
//#endif

//	float		m_dist;
//	float		m_uu;
//	float		m_uv;
//	float		m_vv;
//	uint32		m_nLastTraceCount;

public:
	CollisionTriangle(const Vertex & v0, const Vertex & v1, const Vertex & v2, const IMaterial * pMaterial)
	{
		m_vertices[0] = v0;
		m_vertices[1] = v1;
		m_vertices[2] = v2;
		m_pMaterial = pMaterial;

		m_bbox.vMins = m_bbox.vMaxs = v0.pos;
		m_bbox.AddToBounds(v1.pos);
		m_bbox.AddToBounds(v2.pos);

//		Vec3 boxCenter = m_bbox.Center();
//		m_center = boxCenter;
//		m_extents = boxCenter - m_bbox.vMins;

		m_edgeU = v1.pos - v0.pos;
		m_edgeV = v2.pos - v0.pos;

		//////////////////////////////////////////////////////////////////////////

		m_normal = Vec3::Cross(m_edgeV, m_edgeU);
		m_normal.Normalize();

		int normAxis;
		if (fabs(m_normal.x) > fabs(m_normal.y))
			normAxis = fabs(m_normal.x) > fabs(m_normal.z) ? 0 : 2;
		else
			normAxis = fabs(m_normal.y) > fabs(m_normal.z) ? 1 : 2;
		m_axis_u = normAxis < 2 ? normAxis + 1 : 0;
		m_axis_v = m_axis_u < 2 ? m_axis_u + 1 : 0;

		m_e1u = m_edgeU[m_axis_u];
		m_e1v = m_edgeU[m_axis_v];
		m_e2u = m_edgeV[m_axis_u];
		m_e2v = m_edgeV[m_axis_v];

		float f = (m_e2u * m_e1v - m_e2v * m_e1u);
		if (f != 0.f)
		{
			f = 1.f / f;
			m_e1u *= f;
			m_e1v *= f;
			m_e2u *= f;
			m_e2v *= f;
		}

		//////////////////////////////////////////////////////////////////////////

//		m_normal = Vec3::Cross(m_edgeU, m_edgeV);
//		m_normal.Normalize();
//		m_dist = Vec3::Dot(m_normal, v0.pos);
//
//		m_uu = Vec3::Dot(m_edgeU, m_edgeU);
//		m_uv = Vec3::Dot(m_edgeU, m_edgeV);
//		m_vv = Vec3::Dot(m_edgeV, m_edgeV);
//
//		float d = (m_uv * m_uv - m_uu * m_vv);
////		assert(d != 0.f);
//		if (d != 0.f)
//		{
//			d = 1.f / d;
//			m_uu *= d;
//			m_uv *= d;
//			m_vv *= d;
//		}
	}

	const Vertex & Vertex(int i) const { return m_vertices[i]; }
	const IMaterial * Material() const { return m_pMaterial; }
	const BBox & BoundingBox() const { return m_bbox; }
	const Vec3 & Normal() const { return m_normal; }

//	bool IsDegenerate() const { return (m_normal.Length2() == 0.f) || ((m_uv * m_uv - m_uu * m_vv) == 0.f); }
	bool IsDegenerate() const { return Vec3::Cross(m_edgeU, m_edgeV).LengthSquared() == 0.f; }

//	bool GetParametricCoords(Vec2 & pc, const Vec3 & p) const
//	{
//		const Vec3 w = p - m_vertices[0].pos;
//		const float wu = Vec3::Dot(w, m_edgeU);
//		const float wv = Vec3::Dot(w, m_edgeV);
//
//		// get and test parametric coords
//		pc.x = (m_uv * wv - m_vv * wu);
//		if (pc.x < -0.0001f || pc.x > 1.0001f)
//			return false; // outside
//
//		pc.y = (m_uv * wu - m_uu * wv);
//		if (pc.y < 0.0001f || (pc.x + pc.y) > 1.0001f)
//			return false; //outside
//
//		return true;
//	}

//	void GetParametricCoordsNoCheck(Vec2 & pc, const Vec3 & p) const
//	{
//		const Vec3 w = p - m_vertices[0].pos;
//		const float wu = Vec3::Dot(w, m_edgeU);
//		const float wv = Vec3::Dot(w, m_edgeV);
//		pc.x = (m_uv * wv - m_vv * wu);
//		pc.y = (m_uv * wu - m_uu * wv);
//	}

	Vec3 GetNormal(const Vec2 & pc) const
	{
		return	m_vertices[0].normal * (1.f - pc.x - pc.y) +
				m_vertices[1].normal * pc.x +
				m_vertices[2].normal * pc.y;
	}

	Vec2 GetTexCoord(const Vec2 & pc) const
	{
		return	m_vertices[0].tc * (1.f - pc.x - pc.y) +
				m_vertices[1].tc * pc.x +
				m_vertices[2].tc * pc.y;
	}

	void GetTangents(Vec3 & tangent, Vec3 & binormal, const Vec3 & normal, float bumpDepth) const
	{
		Vec2 dUV1 = m_vertices[1].tc - m_vertices[0].tc;
		Vec2 dUV2 = m_vertices[2].tc - m_vertices[0].tc;
		bumpDepth *= dUV1.x * dUV2.y - dUV1.y * dUV2.x;

		tangent = (m_edgeU * dUV2.y - m_edgeV * dUV1.y);
		tangent *= bumpDepth / tangent.LengthSquared();
		tangent -= normal * Vec3::Dot(tangent, normal);

		binormal = (m_edgeV * dUV1.x - m_edgeU * dUV2.x);
		binormal *= bumpDepth / binormal.LengthSquared();
		binormal -= normal * Vec3::Dot(binormal, normal);
	}

//	bool CheckTraceCount(uint32 nTraceCount)
//	{
//		if (m_nLastTraceCount == nTraceCount)
//			return false;
//	
//		m_nLastTraceCount = nTraceCount;
//		return true;
//	}

//	inline static __m128 cross(const __m128 &a, const __m128 &b)
//	{
//		__m128 result = _mm_sub_ps(_mm_mul_ps(b, _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 0, 2, 1))),
//								   _mm_mul_ps(a, _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 0, 2, 1))));
//		return _mm_shuffle_ps(result, result, _MM_SHUFFLE(3, 0, 2, 1 ));
//	}
//
//	inline static float dot(const __m128 &a, const __m128 &b)
//	{
//		return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
//	}

	bool TraceRay(CollisionRay & ray, TraceResult & tr) const
	{
//		if (!ray.TestIntersection(m_center, m_extents))
//			return false;
//
//		float d1 = Vec3::Dot(ray.Origin(), m_normal) - m_dist;
//		float d2 = Vec3::Dot(ray.End(), m_normal) - m_dist;
//		if (d1 < 0.f)
//		{
//			d1 = -d1;
//			d2 = -d2;
//		}
//		if (d2 >= 0.f)
//			return false;
//
//		float dd = d1 - d2;
//		assert(dd > 0.f);
//		float f = d1 / dd;
//		assert(f >= 0.f && f <= 1.f);
//		Vec3 pos = Vec3::Lerp(ray.Origin(), ray.End(), f);
//
//		Vec2 pc;
//		if (!GetParametricCoords(pc, pos))
//			return false;
//
//		ray.SetEnd(pos);
//		
//		tr.fraction *= f;
//		tr.pTriangle = this;
//		tr.pc = pc;
//
//		if (!((Vec3::Dot(ray.Origin(), m_normal) > m_dist) ^ (Vec3::Dot(ray.End(), m_normal) > m_dist)))
//			return false;

		//////////////////////////////////////////////////////////////////////////

//		Vec3 h = Vec3::Cross(ray.Direction(), m_edgeV);
//		float det = Vec3::Dot(m_edgeU, h);
//		float u, v, t, dt;
//
//		if (det > 0.f)
//		{// back facing triangle
//			Vec3 s = ray.Origin() - m_vertices[0].pos;
//			u = Vec3::Dot(s, h);
//			if (u < 0.f || u > det)
//				return false;
//
//			Vec3 q = Vec3::Cross(s, m_edgeU);
//			v = Vec3::Dot(ray.Direction(), q);
//			if (v < 0.f || u + v > det)
//				return false;
//
//			t = Vec3::Dot(m_edgeV, q); // ray intersection
//			if (t < 0.f || t >= det)
//				return false;
//
//			dt = 1e-6f;
//		}
//		else if (det < 0.f)
//		{// front facing triangle
//			Vec3 s = ray.Origin() - m_vertices[0].pos;
//			u = Vec3::Dot(s, h);
//			if (u > 0.f || u < det)
//				return false;
//
//			Vec3 q = Vec3::Cross(s, m_edgeU);
//			v = Vec3::Dot(ray.Direction(), q);
//			if (v > 0.f || u + v < det)
//				return false;
//
//			t = Vec3::Dot(m_edgeV, q); // ray intersection
//			if (t > 0.f || t <= det)
//				return false;
//
//			dt = -1e-6f;
//		}
//		else
//			return false;
//
//		float invDet = 1.f / det;
//		t = t * invDet + dt;
//		u *= invDet;
//		v *= invDet;

		//////////////////////////////////////////////////////////////////////////

//		if (det == 0.f)
//			return false;
//
//		float invDet = 1.f / det;
//		Vec3 s = ray.Origin() - m_vertices[0].pos;
//		float u = invDet * Vec3::Dot(s, h);
//		if (u < -0.0001f || u > 1.0001f)
//			return(false);
//
//		Vec3 q = Vec3::Cross(s, m_edgeU);
//		float v = invDet * Vec3::Dot(ray.Direction(), q);
//		if (v < -0.0001f || u + v > 1.0001f)
//			return(false);
//
//		// at this stage we can compute t to find out where the intersection point is on the line
//		float t = invDet * Vec3::Dot(m_edgeV, q); // ray intersection
//		if (t < 0.f || t >= 1.f)
//			return false;

		//////////////////////////////////////////////////////////////////////////

//		__m128 h = cross(ray.Direction(), m_vertices[2].pos);
//		float det1 = dot(m_vertices[1].pos, h);
//		if (det1 > 0.f)
//		{// back facing triangle
//			__m128 s = _mm_sub_ps(ray.Origin(), m_vertices[0].pos);
//			float u = dot(s, h);
//			if (u >= 0.0f && u <= det1)
//			{
//				__m128 q = cross(s, m_vertices[1].pos);
//				float v = dot(ray.Direction(), q);
//				if (v >= 0.0f && u + v <= det1)
//				{
//					float t = dot(m_vertices[2].pos, q); // ray intersection
//					if (det1 > t && t >= 0.0f)
//					{
//						*res = (float3)(u, v, t) / det;
//						return true;
//					}
//				}
//			}
//		}

		float det = Vec3::Dot(m_normal, ray.Direction());
		if (det == 0.f)
			return false;

		bool backface = (det > 0.f);
//		if (tr.pTC && !tr.pTC->CheckTriangle(this, backface))
//			return false;

		Vec3 delta = m_vertices[0].pos - ray.Origin();
		float t = Vec3::Dot(m_normal, delta);
		float u, v;

		if (!backface)
		{// front face
			if (t > 0.f || t <= det)
				return false;

			float du = ray.Direction()[m_axis_u] * t - delta[m_axis_u] * det;
			float dv = ray.Direction()[m_axis_v] * t - delta[m_axis_v] * det;

			u = m_e2u * dv - m_e2v * du;
			if (u > 0.f || u < det)
				return false;

			v = m_e1v * du - m_e1u * dv;
			if (v > 0.f || u + v < det)
				return false;

		}
		else
		{// back face
			if (t < 0.f || t >= det)
				return false;

			float du = ray.Direction()[m_axis_u] * t - delta[m_axis_u] * det;
			float dv = ray.Direction()[m_axis_v] * t - delta[m_axis_v] * det;

			u = m_e2u * dv - m_e2v * du;
			if (u < 0.f || u > det)
				return false;

			v = m_e1v * du - m_e1u * dv;
			if (v < 0.f || u + v > det)
				return false;
		}

		float invDet = 1.f / det;
		t *= invDet;
		u *= invDet;
		v *= invDet;
		t += backface ? 1e-6f : -1e-6f;

		//////////////////////////////////////////////////////////////////////////

		ray.Clip(t);

		tr.pTriangle = this;
		tr.fraction *= t;
		tr.backface = backface;
		tr.pc = Vec2(u, v);

		return true;
	}

	Vec2 GetTexCoord(const Vec3 & origin, const Vec3 & direction) const
	{
		Vec3 delta = m_vertices[0].pos - origin;
		float t = Vec3::Dot(m_normal, delta);
		float det = Vec3::Dot(m_normal, direction);

		float du = direction[m_axis_u] * t - delta[m_axis_u] * det;
		float dv = direction[m_axis_v] * t - delta[m_axis_v] * det;

		Vec2 pc(m_e2u * dv - m_e2v * du, m_e1v * du - m_e1u * dv);

		return GetTexCoord(pc / det);
	}
};

typedef std::vector<CollisionTriangle> CollisionTriangleArray;

}
