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
	
struct TraceResult
{
	const class CollisionTriangle * pTriangle;
	float	fraction;
	Vec3	pos;
	Vec2	pc;

	TraceResult() : pTriangle(NULL), fraction(1.f) {}
};

class CollisionTriangle
{
	Vertex		m_vertices[3];
	const void	*m_pUserData;
	BBox		m_bbox;
	Vec3		m_edgeU;
	Vec3		m_edgeV;
//	Vec3		m_normal;
//	float		m_dist;
//	float		m_uu;
//	float		m_uv;
//	float		m_vv;
//	uint32		m_nLastTraceCount;

public:
	CollisionTriangle(const Vertex & v0, const Vertex & v1, const Vertex & v2, const void * pUserData)
//		: m_nLastTraceCount(0)
	{
		m_vertices[0] = v0;
		m_vertices[1] = v1;
		m_vertices[2] = v2;
		m_pUserData = pUserData;

		m_bbox.vMins = m_bbox.vMaxs = v0.pos;
		m_bbox.AddToBounds(v1.pos);
		m_bbox.AddToBounds(v2.pos);

		m_edgeU = v1.pos - v0.pos;
		m_edgeV = v2.pos - v0.pos;

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
	const void * UserData() const { return m_pUserData; }
	const BBox & BoundingBox() const { return m_bbox; }
	const Vec3 & Edge1() const { return m_edgeU; }
	const Vec3 & Edge2() const { return m_edgeV; }
//	const Vec3 & Normal() const { return m_normal; }
//	float Dist() const { return m_dist; }

//	bool IsDegenerate() const { return (m_normal.Length2() == 0.f) || ((m_uv * m_uv - m_uu * m_vv) == 0.f); }
	bool IsDegenerate() const { return Vec3::Cross(m_edgeU, m_edgeV).Length2() == 0.f; }

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

	void GetTangents(Vec3 & tangent, Vec3 & binormal, const Vec3 & normal) const
	{
		Vec2 dUV1 = m_vertices[1].tc - m_vertices[0].tc;
		Vec2 dUV2 = m_vertices[2].tc - m_vertices[0].tc;
		float f = (dUV1.x * dUV2.y - dUV1.y * dUV2.x);
		//f = 1.f / f;

		tangent = (m_edgeU * dUV2.y - m_edgeV * dUV1.y) * f;
		tangent -= normal * Vec3::Dot(tangent, normal);
		tangent.Normalize();

		binormal = (m_edgeV * dUV1.x - m_edgeU * dUV2.x) * f;
		binormal -= normal * Vec3::Dot(binormal, normal);
		binormal.Normalize();
	}

//	bool CheckTraceCount(uint32 nTraceCount)
//	{
//		if (m_nLastTraceCount == nTraceCount)
//			return false;
//	
//		m_nLastTraceCount = nTraceCount;
//		return true;
//	}

	bool TraceRay(CollisionRay & ray, TraceResult & tr) const
	{
//		if (!ray.IsIntersect(m_bbox))
//			return false;

//		float d1 = Vec3::Dot(ray.Start(), m_normal) - m_dist;
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
//		Vec3 pos = Vec3::Lerp(ray.Start(), ray.End(), f);
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

//		if (!((Vec3::Dot(ray.Start(), m_normal) > m_dist) ^ (Vec3::Dot(ray.End(), m_normal) > m_dist)))
//			return false;

		Vec3 h = Vec3::Cross(ray.Direction(), m_edgeV);
		float det = Vec3::Dot(m_edgeU, h);
		float u, v, t, dt;

		if (det > 0.f)
		{// back facing triangle
			Vec3 s = ray.Start() - m_vertices[0].pos;
			u = Vec3::Dot(s, h);
			if (u < 0.f || u > det)
				return false;

			Vec3 q = Vec3::Cross(s, m_edgeU);
			v = Vec3::Dot(ray.Direction(), q);
			if (v < 0.f || u + v > det)
				return false;

			t = Vec3::Dot(m_edgeV, q); // ray intersection
			if (t < 0.f || t >= det)
				return false;

			dt = 1e-6f;
		}
		else if (det < 0.f)
		{// front facing triangle
			Vec3 s = ray.Start() - m_vertices[0].pos;
			u = Vec3::Dot(s, h);
			if (u > 0.f || u < det)
				return false;

			Vec3 q = Vec3::Cross(s, m_edgeU);
			v = Vec3::Dot(ray.Direction(), q);
			if (v > 0.f || u + v < det)
				return false;

			t = Vec3::Dot(m_edgeV, q); // ray intersection
			if (t > 0.f || t <= det)
				return false;

			dt = -1e-6f;
		}
		else
			return false;

		float invDet = 1.f / det;
		t = t * invDet + dt;
		u *= invDet;
		v *= invDet;

//		if (det == 0.f)
//			return false;
//
//		float invDet = 1.f / det;
//		Vec3 s = ray.Start() - m_vertices[0].pos;
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

		ray.Clip(t);

		tr.fraction *= t;
		tr.pTriangle = this;
		tr.pc = Vec2(u, v);

		return true;
	}
};

typedef std::vector<CollisionTriangle> CollisionTriangleArray;

}
