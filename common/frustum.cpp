#include "frustum.h"

using namespace mr;

void Frustum::Create(const Matrix & matViewProj)
{
	// left
	m_planes[0].normal = Vec3(matViewProj.m14 - matViewProj.m11, matViewProj.m24 - matViewProj.m21, matViewProj.m34 - matViewProj.m31);
	m_planes[0].dist = -(matViewProj.m44 - matViewProj.m41);
	// right
	m_planes[1].normal = Vec3(matViewProj.m14 + matViewProj.m11, matViewProj.m24 + matViewProj.m21, matViewProj.m34 + matViewProj.m31);
	m_planes[1].dist = -(matViewProj.m44 + matViewProj.m41);
	// top
	m_planes[2].normal = Vec3(matViewProj.m14 - matViewProj.m12, matViewProj.m24 - matViewProj.m22, matViewProj.m34 - matViewProj.m32);
	m_planes[2].dist = -(matViewProj.m44 - matViewProj.m42);
	// bottom
	m_planes[3].normal = Vec3(matViewProj.m14 + matViewProj.m12, matViewProj.m24 + matViewProj.m22, matViewProj.m34 + matViewProj.m32);
	m_planes[3].dist = -(matViewProj.m44 + matViewProj.m42);
	// near
	m_planes[4].normal = Vec3(matViewProj.m14 - matViewProj.m13, matViewProj.m24 - matViewProj.m23, matViewProj.m34 - matViewProj.m33);
	m_planes[4].dist = -(matViewProj.m44 - matViewProj.m43);
	// far
	m_planes[5].normal = Vec3(matViewProj.m14 + matViewProj.m13, matViewProj.m24 + matViewProj.m23, matViewProj.m34 + matViewProj.m33);
	m_planes[5].dist = -(matViewProj.m44 + matViewProj.m43);

	m_numPlanes = 6;
}

bool Frustum::Test(const Vec3 & point) const
{
	for (int i = 0; i < m_numPlanes; i++)
	{
		if (Vec3::Dot(m_planes[i].normal, point) < m_planes[i].dist)
			return false;
	}
	return true;
}

bool Frustum::Test(const BBox & box) const
{
	for (int i = 0; i < m_numPlanes; i++)
	{
		if (!m_planes[i].TestFront(box))
			return false;
	}
	return true;
}

bool Frustum::Test(const Vec3 & center, float radius) const
{
	for (int i = 0; i < m_numPlanes; i++)
	{
		if (Vec3::Dot(m_planes[i].normal, center) < m_planes[i].dist - radius)
			return false;
	}
	return true;
}
