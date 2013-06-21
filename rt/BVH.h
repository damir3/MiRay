//
//  BVH.h
//  MiRay/rt
//
//  Created by Damir Sagidullin on 01.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

#include "BVHNode.h"

namespace mr
{

class BVH // Bounding Volume Hierarchy
{
	BVHNode m_root;
	CollisionTriangleArray m_triangles;
	uint32 m_nTraceCount;

public:
	BVH(size_t nReserveTrangles = 0) : m_root(NULL), m_nTraceCount(0) { m_triangles.reserve(nReserveTrangles); }

	const BVHNode * Root() const { return &m_root; }
	const CollisionTriangleArray & Triangles() const { return m_triangles; }

	void AddTriangle(const CollisionTriangle & t);
	void Build(byte nMaxNodesLevel = 0);

	bool TraceRay(const Vec3 & vFrom, const Vec3 & vTo, TraceResult & tr);
};

}