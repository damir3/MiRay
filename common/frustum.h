#pragma once

namespace mr
{

class Frustum
{
	struct Plane
	{
		Vec3	normal;
		float	dist;
		
		bool Test(const Vec3 & v) const
		{
			return Vec3::Dot(v, normal) > dist;
		}
		
		bool TestFront(const BBox & box) const
		{
			Vec3 point;
			point.x = (normal.x > 0.f) ? box.vMaxs.x : box.vMins.x;
			point.y = (normal.y > 0.f) ? box.vMaxs.y : box.vMins.y;
			point.z = (normal.z > 0.f) ? box.vMaxs.z : box.vMins.z;
			return Vec3::Dot(point, normal) > dist;
		}
		
		float GetDist(const Vec3 &pos) const
		{
			return Vec3::Dot(normal, pos) - dist;
		}
	};

	enum {maxPlanes = 6};
	int m_numPlanes;
	Plane	m_planes[maxPlanes];
public:
	Frustum() : m_numPlanes(0) {}
	Frustum(const Matrix & matViewProj) { Create(matViewProj); }

	void Clear() { m_numPlanes = 0; }
	void Create(const Matrix & matViewProj);

	bool Test(const Vec3 & point) const;
	bool Test(const BBox & box) const;
	bool Test(const Vec3 & center, float radius) const;
};

}