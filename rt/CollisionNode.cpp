//
//  CollisionNode.cpp
//  MiRay/rt
//
//  Created by Damir Sagidullin on 01.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#include "CollisionNode.h"

using namespace mr;

// ------------------------------------------------------------------------ //

CollisionNode::CollisionNode(CollisionNode * pParent)
	: m_pParent(pParent)
	, m_axis(0)
	, m_dist(FLT_MAX)
{
	m_pChilds[0] = m_pChilds[1] = NULL;
	m_bbox.ClearBounds();
}

CollisionNode::~CollisionNode()
{
	delete m_pChilds[0];
	delete m_pChilds[1];
}

// ------------------------------------------------------------------------ //

size_t CollisionNode::GetNodeCount() const
{
	size_t c = 1;

	if (m_pChilds[0])
		c += m_pChilds[0]->GetNodeCount();

	if (m_pChilds[1])
		c += m_pChilds[1]->GetNodeCount();

	return c;
}

size_t CollisionNode::GetTriangleCount() const
{
	size_t c = m_triangles.size();
	
	if (m_pChilds[0])
		c += m_pChilds[0]->GetTriangleCount();

	if (m_pChilds[1])
		c += m_pChilds[1]->GetTriangleCount();

	return c;
}

size_t CollisionNode::GetChildrenDepth() const
{
	return m_triangles.empty() ? 1 + std::max<size_t>(m_pChilds[0]->GetChildrenDepth(), m_pChilds[1]->GetChildrenDepth()) : 0;
}

// ------------------------------------------------------------------------ //

//struct SplitVolume
//{
//	uint32 numTriangles;
//	BBox bbox;
//
//	void AddTriangle(const BBox & triBBox)
//	{
//		bbox.AddToBounds(triBBox);
//		numTriangles++;
//	}
//
//	float GetSAH() const
//	{
//		return bbox.Area() * numTriangles;
//	}
//};
//
//struct SplitPlane
//{
//	float dist;
//	SplitVolume volume[2];
//	
//	bool GetSide(const BBox &bbox, int axis) const // left = true, right = false;
//	{
//		float dmin = (dist - bbox.vMins[axis]);
//		if (dmin <= 0.f)
//			return false; // right
//		
//		float dmax = (bbox.vMaxs[axis] - dist);
//		if (dmax <= 0.f)
//			return true; // left
//		
//		return (dmax <= dmin);
//	}
//};

bool CollisionNode::FindSplitPlane(const sPolygonArray & polygons)
{
	const Vec3 vBoxSize = m_bbox.Size();
	m_axis = vBoxSize.x >= vBoxSize.y ? (vBoxSize.x >= vBoxSize.z ? 0 : 2) : (vBoxSize.y >= vBoxSize.z ? 1 : 2);
	m_dist = m_bbox.Center()[m_axis];
	return true;
	
//	enum { NUM_SPLIT_VOLUMES = 64, NUM_SPLIT_PLANES = NUM_SPLIT_VOLUMES - 1 };
//	SplitPlane pSplitPlanes[NUM_SPLIT_PLANES * 3];
//	SplitVolume pSplitVolumes[NUM_SPLIT_VOLUMES * 3];
//	
//	{// reset split planes array
//		const Vec3 dSize = vBoxSize / static_cast<float>(NUM_SPLIT_VOLUMES);
//		SplitPlane *pSP = pSplitPlanes;
//		for (int iAxis = 0; iAxis < 3; iAxis++)
//		{
//			for (int vi = 1; vi < NUM_SPLIT_VOLUMES; vi++, pSP++)
//				pSP->dist = m_bbox.vMins[iAxis] + dSize[iAxis] * static_cast<float>(vi);
//		}
//	}
//	
//	{// reset split volumes array
//		SplitVolume * pSV = pSplitVolumes;
//		for (int i = NUM_SPLIT_VOLUMES * 3; i > 0; i--, pSV++)
//		{
//			pSV->numTriangles = 0;
//			pSV->bbox.ClearBounds();
//		}
//	}
//	
//	const Vec3 vScale = Vec3(NUM_SPLIT_VOLUMES / vBoxSize[0], NUM_SPLIT_VOLUMES / vBoxSize[1], NUM_SPLIT_VOLUMES / vBoxSize[2]);
//	for (sPolygonArray::const_iterator it = polygons.begin(), itEnd = polygons.end(); it != itEnd; ++it)
//	{
//		Vec3 vTriangleCenter = it->bbox.Center() - m_bbox.vMins;
//		for (int iAxis = 0; iAxis < 3; iAxis++)
//		{
//			if (vBoxSize[iAxis] == 0.f)
//				continue;
//			
//			int vi = std::min<int>(static_cast<int>(vTriangleCenter[iAxis] * vScale[iAxis]), NUM_SPLIT_PLANES); // volume index
//			assert(vi >= 0 && vi < NUM_SPLIT_VOLUMES);
//			
//			// adjust index value (floating point error correction)
//			if (vi < NUM_SPLIT_PLANES && !pSplitPlanes[iAxis * NUM_SPLIT_PLANES + vi].GetSide(it->bbox, iAxis))
//				vi++;
//			else if (vi > 0 && pSplitPlanes[iAxis * NUM_SPLIT_PLANES + vi - 1].GetSide(it->bbox, iAxis))
//				vi--;
//			
//			pSplitVolumes[iAxis * NUM_SPLIT_VOLUMES + vi].AddTriangle(it->bbox);
//		}
//	}
//	
//	for (int iAxis = 0; iAxis < 3; iAxis++)
//	{
//		if (vBoxSize[iAxis] == 0.f)
//			continue;
//		
//		SplitPlane * pSP = pSplitPlanes + iAxis * NUM_SPLIT_PLANES;
//		const SplitVolume * pSV = pSplitVolumes + iAxis * NUM_SPLIT_VOLUMES;
//		for (int i = 0; i < NUM_SPLIT_PLANES; i++)
//		{
//			pSP[i].volume[1].numTriangles = pSV[i].numTriangles;
//			pSP[i].volume[1].bbox = pSV[i].bbox;
//			
//			int ri = NUM_SPLIT_PLANES - 1 - i; // reverse index
//			pSP[ri].volume[0].numTriangles = pSV[ri + 1].numTriangles;
//			pSP[ri].volume[0].bbox = pSV[ri + 1].bbox;
//			
//			if (i > 0)
//			{
//				pSP[i].volume[1].numTriangles += pSP[i - 1].volume[1].numTriangles;
//				pSP[i].volume[1].bbox.AddToBounds(pSP[i - 1].volume[1].bbox);
//				
//				pSP[ri].volume[0].numTriangles += pSP[ri + 1].volume[0].numTriangles;
//				pSP[ri].volume[0].bbox.AddToBounds(pSP[ri + 1].volume[0].bbox);
//			}
//		}
//	}
//	
//	m_axis = -1;
//	m_dist = 0.f;
//	float fBestCost = FLT_MAX;
//	for (int iAxis = 0; iAxis < 3; iAxis++)
//	{
//		if (vBoxSize[iAxis] == 0.f)
//			continue;
//		
//		const SplitPlane * pSP = pSplitPlanes + iAxis * NUM_SPLIT_PLANES;
//		for (int pi = 0; pi < NUM_SPLIT_PLANES; pi++, pSP++)
//		{
//			if (pSP->volume[0].numTriangles && pSP->volume[1].numTriangles)
//			{
//				float fCost = pSP->volume[0].GetSAH() + pSP->volume[1].GetSAH();
//				if (fBestCost > fCost)
//				{
//					fBestCost = fCost;
//					m_axis = iAxis;
//					m_dist = pSP->dist;
//				}
//			}
//		}
//	}
//	
//	return m_axis >= 0;
}

//struct SplitPlane
//{
//	float dist;
//	SplitVolume volume[2];
//};

//bool CollisionNode::FindSplitPlane(const sPolygonArray & polygons)
//{
//	const Vec3 vBoxSize = m_bbox.Size();
//	m_axis = vBoxSize.x >= vBoxSize.y ? (vBoxSize.x >= vBoxSize.z ? 0 : 2) : (vBoxSize.y >= vBoxSize.z ? 1 : 2);
//	m_dist = m_bbox.Center()[m_axis];
//	return true;
//
//	enum { NUM_SPLIT_VOLUMES = 64, NUM_SPLIT_PLANES = NUM_SPLIT_VOLUMES - 1 };
//	SplitPlane pSplitPlanes[3][NUM_SPLIT_PLANES];
//
//	{// reset split planes array
//		const Vec3 dSize = vBoxSize / static_cast<float>(NUM_SPLIT_VOLUMES);
//		for (int iAxis = 0; iAxis < 3; iAxis++)
//		{
//			SplitPlane * pSP = pSplitPlanes[iAxis];
//			for (int vi = 0; vi < NUM_SPLIT_PLANES; vi++, pSP++)
//			{
//				pSP->dist = m_bbox.vMins[iAxis] + dSize[iAxis] * static_cast<float>(vi + 1);
//				pSP->volume[0].numTriangles = 0;
//				pSP->volume[0].bbox.ClearBounds();
//				pSP->volume[1].numTriangles = 0;
//				pSP->volume[1].bbox.ClearBounds();
//			}
//		}
//	}
//
//	const Vec3 vScale = Vec3(NUM_SPLIT_VOLUMES / vBoxSize[0], NUM_SPLIT_VOLUMES / vBoxSize[1], NUM_SPLIT_VOLUMES / vBoxSize[2]);
//	for (sPolygonArray::const_iterator it = polygons.begin(), itEnd = polygons.end(); it != itEnd; ++it)
//	{
//		for (int iAxis = 0; iAxis < 3; iAxis++)
//		{
//			if (vBoxSize[iAxis] == 0.f)
//				continue;
//
//			int minVI = static_cast<int>((it->bbox.vMins[iAxis] - m_bbox.vMins[iAxis]) * vScale[iAxis]); // volume index
//			if (minVI < NUM_SPLIT_PLANES - 1)
//				pSplitPlanes[iAxis][minVI].volume[0].AddTriangle(it->bbox);
//
//			int maxVI = static_cast<int>((it->bbox.vMaxs[iAxis] - m_bbox.vMins[iAxis]) * vScale[iAxis]); // volume index
//			if (maxVI > 1)
//				pSplitPlanes[iAxis][maxVI - 1].volume[1].AddTriangle(it->bbox);
//		}
//	}
//
//	for (int iAxis = 0; iAxis < 3; iAxis++)
//	{
//		if (vBoxSize[iAxis] == 0.f)
//			continue;
//
//		SplitPlane * pSP = pSplitPlanes[iAxis];
//		for (int i = 1; i < NUM_SPLIT_PLANES; i++)
//		{
//			pSP[i].volume[0].numTriangles += pSP[i - 1].volume[0].numTriangles;
//			pSP[i].volume[0].bbox.AddToBounds(pSP[i - 1].volume[0].bbox);
//		}
//		for (int i = NUM_SPLIT_PLANES - 1; i > 0; i--)
//		{
//			pSP[i - 1].volume[1].numTriangles += pSP[i].volume[1].numTriangles;
//			pSP[i - 1].volume[1].bbox.AddToBounds(pSP[i].volume[1].bbox);
//		}
//	}
//
//	m_axis = -1;
//	m_dist = 0.f;
//	float fBestCost = FLT_MAX;
//	for (int iAxis = 0; iAxis < 3; iAxis++)
//	{
//		if (vBoxSize[iAxis] == 0.f)
//			continue;
//
//		const SplitPlane * pSP = pSplitPlanes[iAxis];	
//		for (int pi = 0; pi < NUM_SPLIT_PLANES; pi++, pSP++)
//		{
////			if (!pSP->volume[0].numTriangles || !pSP->volume[1].numTriangles)
////				continue;
//
//			float fCost = pSP->volume[0].GetSAH() + pSP->volume[1].GetSAH();
//			if (fBestCost > fCost)
//			{
//				fBestCost = fCost;
//				m_axis = iAxis;
//				m_dist = pSP->dist;
//			}
//		}
//	}
//
//	return m_axis >= 0;
//}

void CollisionNode::FillTriangles(const sPolygonArray & polygons)
{
	for (sPolygonArray::const_iterator it = polygons.begin(), itEnd = polygons.end(); it != itEnd; ++it)
		m_triangles.push_back(it->pTriangle);
}

BBox CollisionNode::CalculateBBox(const sPolygonArray & polygons)
{
	BBox bbox;
	bbox.ClearBounds();
	for (sPolygonArray::const_iterator it = polygons.begin(), itEnd = polygons.end(); it != itEnd; ++it)
		bbox.AddToBounds(it->bbox);
	return bbox;
}

void CollisionNode::Create(byte level, const sPolygonArray & polygons)
{
	m_bbox = CalculateBBox(polygons);
	Vec3 boxCenter = m_bbox.Center();
	m_center = boxCenter;
	m_extents = boxCenter - m_bbox.vMins;

	if (level == 0 || polygons.size() <= MAX_NODE_TRIANGLES || !FindSplitPlane(polygons))
	{
		FillTriangles(polygons);
		return;
	}

	float fMinSize = FLT_MAX;
	for (sPolygonArray::const_iterator it = polygons.begin(), itEnd = polygons.end(); it != itEnd; ++it)
	{
		const BBox & bbox = it->pTriangle->BoundingBox();
		fMinSize = std::min<float>(fMinSize, bbox.vMaxs[m_axis] - bbox.vMins[m_axis]);
	}

	if ((m_bbox.vMaxs[m_axis] - m_bbox.vMins[m_axis]) < (fMinSize * 2.f))
	{
		FillTriangles(polygons);
		return;
	}

	sPolygonArray childPolygons[2];
	childPolygons[0].reserve(polygons.size());
	childPolygons[1].reserve(polygons.size());

	for (sPolygonArray::const_iterator it = polygons.begin(), itEnd = polygons.end(); it != itEnd; ++it)
	{
		if (it->bbox.vMaxs[m_axis] <= m_dist)
			childPolygons[0].push_back(*it);
		else if (it->bbox.vMins[m_axis] >= m_dist)
			childPolygons[1].push_back(*it);
		else
		{// split polygon
			sPolygon p1(it->pTriangle);
			sPolygon p2(it->pTriangle);

			float fDist1 = it->vertices[0][m_axis] - m_dist;
			for (int i = 0; i < it->numVertices; i++)
			{
				const Vec3 & v1 = it->vertices[i];
				const Vec3 & v2 = it->vertices[i < it->numVertices-1 ? i + 1 : 0];
				if (fDist1 > 0.f)
					p2.AddVertex(v1);
				else
					p1.AddVertex(v1);

				float fDist2 = v2[m_axis] - m_dist;
				if ((fDist1 > 0.f) ^ (fDist2 > 0.f))
				{
					float f = fDist1 / (fDist1 - fDist2);
					Vec3 clip = Vec3::Lerp(v1, v2, f);
					clip[m_axis] = m_dist; // interpolation error correction
					p1.AddVertex(clip);
					p2.AddVertex(clip);
				}

				fDist1 = fDist2;
			}

			assert(p1.numVertices >= 3);
			assert(p2.numVertices >= 3);

			if (p1.numVertices >= 3)
				childPolygons[0].push_back(p1);

			if (p2.numVertices >= 3)
				childPolygons[1].push_back(p2);
		}
	}

	assert(!childPolygons[0].empty() && !childPolygons[1].empty());
	if (childPolygons[0].empty() || childPolygons[1].empty())
	{
		FillTriangles(polygons);
		return;
	}
	
//	if ((CalculateBBox(childPolygons[0]).Area() * childPolygons[0].size()) +
//		(CalculateBBox(childPolygons[1]).Area() * childPolygons[1].size()) >
//		(m_bbox.Area() * polygons.size()))
//	{
//		FillTriangles(polygons);
//		return;
//	}

	level--;

	m_pChilds[0] = new CollisionNode(this);
	m_pChilds[0]->Create(level, childPolygons[0]);

	m_pChilds[1] = new CollisionNode(this);
	m_pChilds[1]->Create(level, childPolygons[1]);

	if (polygons.size() <= GetChildrenDepth() * 2)
	{
		delete m_pChilds[0];
		m_pChilds[0] = NULL;
		delete m_pChilds[1];
		m_pChilds[1] = NULL;

		FillTriangles(polygons);
	}
}
