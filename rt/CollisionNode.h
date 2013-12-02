//
//  CollisionNode.h
//  MiRay/rt
//
//  Created by Damir Sagidullin on 01.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

#include "CollisionTriangle.h"

namespace mr
{

class CollisionNode
{
public:
	struct sPolygon
	{
		CollisionTriangle * pTriangle;
		BBox bbox;
		int  numVertices;
		Vec3 vertices[9];

		sPolygon(CollisionTriangle * pOriginTriangle)
			: pTriangle(pOriginTriangle)
			, numVertices(0)
		{
			bbox.ClearBounds();
		}

//		sPolygon(const sPolygon & p) : pTriangle(p.pTriangle), bbox(p.bbox), numVertices(p.numVertices)
//		{
//			memcpy(vertices, p.vertices, numVertices * sizeof(Vec3));
//		}

		void AddVertex(const Vec3 & p)
		{
			assert(numVertices < 9);
			vertices[numVertices++] = p;
			bbox.AddToBounds(p);
		}
	};

	typedef std::vector<sPolygon> sPolygonArray;

private:
	enum { MAX_NODE_TRIANGLES = 8 };

	CollisionNode * const m_pParent;

	BBox		m_bbox;
	Vec3		m_center;
	Vec3		m_extents;
	int			m_axis;
	float		m_dist;

	CollisionNode *	m_pChilds[2];
	std::vector<CollisionTriangle *>	m_triangles;

	bool FindSplitPlane(const sPolygonArray & polygons);
	void FillTriangles(const sPolygonArray & polygons);
	static BBox CalculateBBox(const sPolygonArray & polygons);
	static float CalculateSAH(const sPolygonArray & polygons);

public:
	CollisionNode(CollisionNode * pParent);
	~CollisionNode();

	const BBox & BoundingBox() const { return m_bbox; }
	const Vec3 & Center() const { return m_center; }
	const Vec3 & Extents() const { return m_extents; }
	byte Axis() const { return m_axis; }
	float Dist() const { return m_dist; }
	CollisionNode * Child(int i) const { return m_pChilds[i]; }
	const std::vector<CollisionTriangle *> & Triangles() const { return m_triangles; }

	size_t GetNodeCount() const;
	size_t GetTriangleCount() const;
	size_t GetChildrenDepth() const;

	void Create(byte level, const sPolygonArray & polygons);
};

}
