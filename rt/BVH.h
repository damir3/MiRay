//
//  BVH.h
//  MiRay/rt
//
//  Created by Damir Sagidullin on 01.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

#include "CollisionNode.h"
#include "CollisionVolume.h"
#include <atomic>

namespace mr
{

class BVH // Bounding Volume Hierarchy
{
//	CollisionNode m_root;
//	CollisionTriangleArray m_triangles;

	typedef std::vector<CollisionVolume *>	CollisionVolumeArray;
	CollisionVolumeArray m_volumes;
	std::atomic<size_t>	m_nRayCounter;

public:
//	BVH(size_t nReserveTrangles = 0) : m_root(NULL), m_nTraceCount(0) { m_triangles.reserve(nReserveTrangles); }
	BVH();
	~BVH();

	CollisionVolume * CreateVolume(size_t nReserveTrangles = 0);
	void DestroyVolume(CollisionVolume *);

	size_t NumVolumes() const { return m_volumes.size(); }
	CollisionVolume * Volume(size_t i) { return m_volumes[i]; }
	const CollisionVolume * Volume(size_t i) const { return m_volumes[i]; }

	BBox BoundingBox() const;
	
//	const CollisionNode * Root() const { return &m_root; }
//	const CollisionTriangleArray & Triangles() const { return m_triangles; }
//
//	void AddTriangle(const CollisionTriangle & t);
//	void Build(byte nMaxNodesLevel = 0);

	size_t RaysCounter() const { return m_nRayCounter; }
	void ResetRayCounter() { m_nRayCounter = 0; }

	bool TraceRay(const Vec3 & vFrom, const Vec3 & vTo, TraceResult & tr);
};

}