//
//  BVH.cpp
//  MiRay/rt
//
//  Created by Damir Sagidullin on 01.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#include "BVH.h"

using namespace mr;

void BVH::AddTriangle(const CollisionTriangle & t)
{
	m_triangles.push_back(t);
}

void BVH::Build(byte nMaxNodesLevel)
{
	if (nMaxNodesLevel == 0)
	{
		while ((nMaxNodesLevel < 32) && ((size_t)(1 << nMaxNodesLevel) < m_triangles.size()))
			nMaxNodesLevel++;
	}

	if (nMaxNodesLevel > 0)
	{
		BVHNode::sPolygonArray polygons;
		polygons.reserve(m_triangles.size());
		for (CollisionTriangleArray::iterator it = m_triangles.begin(), itEnd = m_triangles.end(); it != itEnd; ++it)
		{
			polygons.push_back(BVHNode::sPolygon(&(*it)));
			polygons.back().AddVertex(it->Vertex(0).pos);
			polygons.back().AddVertex(it->Vertex(1).pos);
			polygons.back().AddVertex(it->Vertex(2).pos);
		}

		m_root.Create(nMaxNodesLevel - 1, polygons);
	}
}

bool BVH::TraceRay(const Vec3 & vFrom, const Vec3 & vTo, TraceResult & tr)
{
	m_nTraceCount++;
	const CollisionTriangle * pSkipTriangle = tr.pTriangle;

	CollisionRay ray(vFrom, vTo);

	const BVHNode * pStackNodes[64]; // nMaxNodesLevel must be less than 64
	const BVHNode ** ppTopNode = pStackNodes;
	*ppTopNode++ = &m_root;

	while (ppTopNode > pStackNodes)
	{
		const BVHNode * pNode = *(--ppTopNode);
		if (!ray.TestIntersection(pNode->Center(), pNode->Extents()))
			continue;

		if (pNode->Triangles().empty())
		{
			assert(pNode->Child(0) && pNode->Child(1));
			if (ray.Start()[pNode->Axis()] > pNode->Dist())
			{
				*ppTopNode++ = pNode->Child(0);
				*ppTopNode++ = pNode->Child(1);
			}
			else
			{
				*ppTopNode++ = pNode->Child(1);
				*ppTopNode++ = pNode->Child(0);
			}
		}
		else
		{
			for (std::vector<CollisionTriangle *>::const_iterator it = pNode->Triangles().begin(), itEnd = pNode->Triangles().end(); it != itEnd; ++it)
			{
				CollisionTriangle * pTriangle = *it;
//				if (pTriangle->CheckTraceCount(m_nTraceCount) && pTriangle->TraceRay(ray, tr))
				if (tr.pTriangle != pTriangle && pTriangle->TraceRay(ray, tr))
				{
					assert(tr.pTriangle == pTriangle);
				}
			}

			if (tr.pTriangle != pSkipTriangle && pNode->BoundingBox().Test(ray.End()))
//			if (tr.pTriangle != pSkipTriangle)
				break;
		}
	}

	tr.pos = ray.End();

	return tr.pTriangle != pSkipTriangle;
}
