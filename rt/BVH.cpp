//
//  BVH.cpp
//  MiRay/rt
//
//  Created by Damir Sagidullin on 01.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#include "BVH.h"

using namespace mr;

// ------------------------------------------------------------------------ //

BVH::BVH() : m_nRayCounter(0)
{
}

BVH::~BVH()
{
	while (!m_volumes.empty())
	{
		delete m_volumes.back();
		m_volumes.pop_back();
	}
}

// ------------------------------------------------------------------------ //

CollisionVolume * BVH::CreateVolume(size_t nReserveTrangles)
{
	m_volumes.push_back(new CollisionVolume(nReserveTrangles));
	return m_volumes.back();
}

void BVH::DestroyVolume(CollisionVolume * pVolume)
{
	std::vector<CollisionVolume *>::iterator it = std::find(m_volumes.begin(), m_volumes.end(), pVolume);
	if (it != m_volumes.end())
	{
		delete pVolume;
		m_volumes.erase(it);
	}
}

// ------------------------------------------------------------------------ //

BBox BVH::BoundingBox() const
{
	BBox bbox;
	bbox.ClearBounds();
	for (CollisionVolumeArray::const_iterator it = m_volumes.begin(), itEnd = m_volumes.end(); it != itEnd; ++it)
		bbox.AddToBounds((*it)->AABB());

	return bbox;
}

// ------------------------------------------------------------------------ //

bool BVH::TraceRay(const Vec3 & vFrom, const Vec3 & vTo, TraceResult & tr)
{
	++m_nRayCounter;

	tr.pos = vTo;
	bool res = false;
	for (CollisionVolumeArray::iterator it = m_volumes.begin(), itEnd = m_volumes.end(); it != itEnd; ++it)
		res |= (*it)->TraceRay(vFrom, tr.pos, tr);

	return res;
}
