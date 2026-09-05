/*
	Copyright (C) 2013-2020 Damir Sagidullin

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
	documentation files (the "Software"), to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions
	of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
	TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
	THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
	DEALINGS IN THE SOFTWARE.

	(The above is MIT License: http://en.wikipedia.origin/wiki/MIT_License)
*/

#pragma once

#include "../../Shared/Interfaces/MeshNode.h"
#include "Ray.h"
#include "RenderLayerImpl.h"

#define USE_INSTANCE_SCENE_GEOMETRY

class MeshNode final : public Node, public IMeshNode
{
	Q_OBJECT
	Q_INTERFACES(IMeshNode)

	RenderLayerEnumParamImpl	m_renderLayer;

	GeometriesList	m_geometries;
	RTCScene		m_rtcScene;
	RTCGeometry 	m_rtcInstance;
	unsigned int	m_instID;
	BBox			m_bboxGeom;

	void updateVisibility(bool visible) override;
	void updateTransformation() override;
	void calculateBoundingBox() override;
	void collectBottomVertices(float threshold, vec2 & sumXY, int & count) const override;

	void createMeshCollisionGeometries();
	void destroyMeshCollisionGeometries();

public:
	MeshNode(const QString & name, Scene & scene, Node * parent);
	~MeshNode() override;

	eSceneElementType type() const override { return SceneElement_MeshNode; }
	bool isMeshNode() const override { return true; }

	RenderLayerEnumParamImpl & renderLayer() override { return m_renderLayer; }

	void getStateRecursive(QJsonObject & nodes, QJsonObject & geoms, bool fullInfo) override;
	void setStateRecursive(const QJsonObject & nodes, const QJsonObject & geoms, uint32_t flags) override;

	void setAnimationStateRecursive(const QJsonObject & nodes1, const QJsonObject & nodes2, const QJsonObject & geoms1, const QJsonObject & geoms2, uint32_t flags) override;
	void setAnimationTimeRecursive(float t, Sampler & sampler) override;

	size_t numChildren(eSceneElementType type) const override;
	ISceneElement * child(size_t i, eSceneElementType type) const override;

	Geometry *addGeometry(IMaterial * material) override;

	void insertGeometry(GeometryPtr & geom, int pos);
	GeometryPtr removeGeometry(size_t pos); // returns removed geometry

	size_t numGeometries() const override { return m_geometries.size(); }
	Geometry * getGeometry(size_t i) const override { return m_geometries[i].get(); }
	int getGeometryIndex(const IGeometry * geom) const override;
	const GeometriesList & geometries() const { return m_geometries; }

	RTCDevice rtcDevice() const;
#ifdef USE_INSTANCE_SCENE_GEOMETRY
	RTCScene rtcScene() const { return m_rtcScene; }
	unsigned getInstanceID() const { return m_instID; }
#else
	RTCScene rtcScene() const { return m_scene.rtcScene(); }
#endif

	static MeshNode * getByRay(RTCScene scene, const Ray & ray)
	{
		if (auto rtcInstance = rtcGetGeometry(scene, ray.hit.instID[0])) {
			assert(rtcGetGeometryUserData(rtcInstance) != nullptr);
			return static_cast<MeshNode *>(rtcGetGeometryUserData(rtcInstance));
		}
		return nullptr;
	}

	Geometry * getGeometryByRay(const Ray & ray) const
	{
		if (auto rtcGeom = rtcGetGeometry(m_rtcScene, ray.hit.geomID)) {
			assert(rtcGetGeometryUserData(rtcGeom) != nullptr);
			return static_cast<Geometry *>(rtcGetGeometryUserData(rtcGeom));
		}
		return nullptr;
	}

	size_t numTriangles() const;

	const BBox & geomBBox() const { return m_bboxGeom; }
	void addToAABB(BBox & bbox, bool checkVisibility) const override;

	void createCollisionGeometries() override;
	void destroyCollisionGeometries() override;

	void updateMeshCollisionGeometries();

	bool isMaterialUsed(const IMaterial * material) const override;
	void findGeometriesByMaterial(std::vector<IGeometry *> & geometries, const IMaterial * material) const override;

	void findMeshNodesByRenderLayer(std::vector<MeshNode *> & meshNodes, IRenderLayer * renderLayer) override;
	void findLights(std::vector<AbstractLight *> & lights, bool includeGeomLights) override;
};
