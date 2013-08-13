//
//  CollisionVolume.h
//  MiRay/rt
//
//  Created by Damir Sagidullin on 08.08.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

#include "CollisionTriangle.h"

namespace mr
{

class CollisionVolume
{
	CollisionNode m_root;
	uint32	m_nTraceCount;
	Matrix	m_matTransformation;
	Matrix	m_matInvTransformation;
	BBox	m_aabb;
	CollisionTriangleArray m_triangles;

public:
	CollisionVolume(size_t nReserveTrangles = 0);

	const Matrix & Transformation() const { return m_matTransformation; }
	const Matrix & InverseTransformation() const { return m_matInvTransformation; }
	void SetTransformation(const Matrix & m);

	const CollisionNode * Root() const { return &m_root; }
	const CollisionTriangleArray & Triangles() const { return m_triangles; }

	const BBox & AABB() const { return m_aabb; }
	const BBox & OOBB() const { return m_root.BoundingBox(); }

	void AddTriangle(const CollisionTriangle & t);
	void Build(byte nMaxNodesLevel = 0);

	bool TraceRay(const Vec3 & vFrom, const Vec3 & vTo, TraceResult & tr);
};

}
