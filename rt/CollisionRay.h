//
//  CollisionRay.h
//  MiRay/rt
//
//  Created by Damir Sagidullin on 02.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

namespace mr
{

class CollisionRay
{
	Vec3 m_vOrigin;
	Vec3 m_vEnd;

	Vec3 m_vDir;
	Vec3 m_vCenter;
	Vec3 m_vHalfDir;
	Vec3 m_vHalfSize;
	
public:
	
	CollisionRay(const Vec3 & origin, const Vec3 & end)
		: m_vOrigin(origin)
		, m_vEnd(end)
		, m_vDir(end - origin)
	{
		m_vHalfDir = m_vDir * 0.5f;
		m_vCenter = m_vOrigin + m_vHalfDir;
		m_vHalfSize = Vec3(fabs(m_vHalfDir.x), fabs(m_vHalfDir.y), fabs(m_vHalfDir.z));
	}

	const Vec3 & Origin() const { return m_vOrigin; }
	const Vec3 & End() const { return m_vEnd; }
	const Vec3 & Direction() const { return m_vDir; }

	void Clip(float f)
	{
		m_vDir *= f;
		m_vEnd = m_vOrigin + m_vDir;
		m_vHalfDir *= f;
		m_vCenter = m_vOrigin + m_vHalfDir;
		m_vHalfSize *= f;
	}

//	void SetEnd(const Vec3 & vEnd)
//	{
//		m_vEnd = vEnd;
//		m_vDir = m_vEnd - m_vOrigin;
//		m_vHalfDir = m_vDir * 0.5f;
//		m_vCenter = m_vOrigin + m_vHalfDir;
//		m_vHalfSize = Vec3(fabs(m_vHalfDir.x), fabs(m_vHalfDir.y), fabs(m_vHalfDir.z));
//	}

//	bool TestIntersection(const BBox & bbox) const
//	{
//		Vec3 vBoxCenter;
//		Vec3 vBoxExtents;
//		Vec3 vDelta;
//		vBoxCenter.x = (bbox.vMins.x + bbox.vMaxs.x) * 0.5f;
//		vBoxExtents.x = (vBoxCenter.x - bbox.vMins.x);
//		vDelta.x = m_vCenter.x - vBoxCenter.x;
//		if (fabsf(vDelta.x) > vBoxExtents.x + m_vHalfSize.x)
//			return false;
//		
//		vBoxCenter.y = (bbox.vMins.y + bbox.vMaxs.y) * 0.5f;
//		vBoxExtents.y = (vBoxCenter.y - bbox.vMins.y);
//		vDelta.y = m_vCenter.y - vBoxCenter.y;
//		if (fabsf(vDelta.y) > vBoxExtents.y + m_vHalfSize.y)
//			return false;
//		
//		vBoxCenter.z = (bbox.vMins.z + bbox.vMaxs.z) * 0.5f;
//		vBoxExtents.z = (vBoxCenter.z - bbox.vMins.z);
//		vDelta.z = m_vCenter.z - vBoxCenter.z;
//		if (fabsf(vDelta.z) > vBoxExtents.z + m_vHalfSize.z)
//			return false;
//	
//		if (fabsf((m_vHalfDir.y * vDelta.z) - (m_vHalfDir.z * vDelta.y)) > (vBoxExtents.y * m_vHalfSize.z + vBoxExtents.z * m_vHalfSize.y))
//			return false;
//		
//		if (fabsf((m_vHalfDir.z * vDelta.x) - (m_vHalfDir.x * vDelta.z)) > (vBoxExtents.z * m_vHalfSize.x + vBoxExtents.x * m_vHalfSize.z))
//			return false;
//		
//		if (fabsf((m_vHalfDir.x * vDelta.y) - (m_vHalfDir.y * vDelta.x)) > (vBoxExtents.x * m_vHalfSize.y + vBoxExtents.y * m_vHalfSize.x))
//			return false;
//		
//		return true;
//	}

	bool TestIntersection(const Vec3 & vBoxCenter, const Vec3 & vBoxExtents) const
	{
		Vec3 vDelta = m_vCenter - vBoxCenter;
		return
			(fabsf((m_vHalfDir.y * vDelta.z) - (m_vHalfDir.z * vDelta.y)) <= (vBoxExtents.y * m_vHalfSize.z + vBoxExtents.z * m_vHalfSize.y)) &&
			(fabsf((m_vHalfDir.z * vDelta.x) - (m_vHalfDir.x * vDelta.z)) <= (vBoxExtents.z * m_vHalfSize.x + vBoxExtents.x * m_vHalfSize.z)) &&
			(fabsf((m_vHalfDir.x * vDelta.y) - (m_vHalfDir.y * vDelta.x)) <= (vBoxExtents.x * m_vHalfSize.y + vBoxExtents.y * m_vHalfSize.x));
	}
};

}