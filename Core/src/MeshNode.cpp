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

#include "MeshNode.h"
#include "Scene.h"

// ------------------------------------------------------------------------ //

MeshNode::MeshNode(const QString & name, Scene & scene, Node * parent)
	: Node(name, scene, parent)
	, m_renderLayer(scene.renderLayerManager(), PID_MESH_NODE_RENDER_LAYER, *this)
	, m_rtcScene(nullptr)
	, m_rtcInstance(nullptr)
	, m_instID(RTC_INVALID_GEOMETRY_ID)
{
}

MeshNode::~MeshNode()
{
	destroyMeshCollisionGeometries();
}

// ------------------------------------------------------------------------ //

RTCDevice MeshNode::rtcDevice() const
{
	return m_scene.rtcDevice();
}

// ------------------------------------------------------------------------ //

void MeshNode::getStateRecursive(QJsonObject & nodes, QJsonObject & geoms, bool fullInfo)
{
	Node::getStateRecursive(nodes, geoms, fullInfo);

	for (const auto & geom : m_geometries) {
		geoms[geom->guid()] = geom->getState(fullInfo);
	}
}

void MeshNode::setStateRecursive(const QJsonObject & nodes, const QJsonObject & geoms, uint32_t flags)
{
	Node::setStateRecursive(nodes, geoms, flags);

	if (flags & SF_HAS_ASSIGNED_MATERIALS) {
		for (const auto & geom : m_geometries) {
			if (geoms.contains(geom->guid()))
				geom->setState(geoms[geom->guid()].toObject());
		}
	}
}

void MeshNode::setAnimationStateRecursive(const QJsonObject & nodes1, const QJsonObject & nodes2, const QJsonObject & geoms1, const QJsonObject & geoms2, uint32_t flags)
{
	Node::setAnimationStateRecursive(nodes1, nodes2, geoms1, geoms2, flags);

	for (const auto & geom : m_geometries)
		geom->setAnimationState(geoms1, geoms2, flags);
}

void MeshNode::setAnimationTimeRecursive(float t, Sampler & sampler)
{
	Node::setAnimationTimeRecursive(t, sampler);

	for (const auto & geom : m_geometries)
		geom->setAnimationTime(t, sampler);
}

// ------------------------------------------------------------------------ //

size_t MeshNode::numChildren(eSceneElementType type) const
{
	return Node::numChildren(type) + (type & SceneElement_Geometry ? numGeometries() : 0);
}

ISceneElement * MeshNode::child(size_t i, eSceneElementType type) const
{
	auto numChildren = Node::numChildren(type);
	if (i < numChildren)
		return Node::child(i, type);

	return type & SceneElement_Geometry ? m_geometries[i - numChildren].get() : nullptr;
}

Geometry *MeshNode::addGeometry(IMaterial * material)
{
	auto geometry = new Geometry(this, m_scene.createGuid());
	m_geometries.emplace_back(geometry);
	geometry->setMaterial(material ? material : m_scene.materialManager().getDefault());
	return geometry;
}

void MeshNode::insertGeometry(GeometryPtr & geom, int pos)
{
	auto it = (pos < 0 || pos >= static_cast<int>(m_geometries.size())) ? m_geometries.end() : m_geometries.begin() + pos;

	if (!geom->meshNode()) {
		geom->setMeshNode(this);
	} else if (geom->meshNode() != this) {
		assert(false && "can't insert other geometry");
		return;
	}

	if (m_rtcScene && geom->numTriangles() > 0) {
		assert(geom->geomID() == RTC_INVALID_GEOMETRY_ID);
		geom->createCollisionGeometry();
		rtcCommitScene(m_rtcScene);
		rtcCommitGeometry(m_rtcInstance);
		rtcCommitScene(m_scene.rtcScene());
//		rtcSceneUpdate(m_scene.rtcScene(), m_instID);
	}

	m_geometries.insert(it, std::move(geom));
	calculateBoundingBox();

	m_scene.fireNodeChanged(this, NodeChanged_ChildrenList);
}

GeometryPtr MeshNode::removeGeometry(size_t pos)
{
	assert(pos < m_geometries.size());
	auto it = m_geometries.begin() + pos;
	auto * geometry = it->get();

	if (m_rtcScene) {
		geometry->destroyCollisionGeometry();
		rtcCommitScene(m_rtcScene);
		rtcCommitGeometry(m_rtcInstance);
		rtcCommitScene(m_scene.rtcScene());
//		rtcUpdate(m_scene.rtcScene(), m_instID);
	}

	geometry->setMeshNode(nullptr);

	GeometryPtr geom = std::move(*it);
	m_geometries.erase(it);
	calculateBoundingBox();

	m_scene.fireNodeChanged(this, NodeChanged_ChildrenList);

	return geom;
}

// ------------------------------------------------------------------------ //

int MeshNode::getGeometryIndex(const IGeometry *geometry) const
{
	auto it = std::find_if(m_geometries.begin(), m_geometries.end(),
		[geometry](const GeometryPtr & gp) { return gp.get() == geometry; });
	if (it == m_geometries.end())
		return -1;

	return static_cast<int>(std::distance(m_geometries.begin(), it));
}

// ------------------------------------------------------------------------ //

size_t MeshNode::numTriangles() const
{
	size_t count = 0;
	for (const auto & geom : m_geometries)
		count += geom->numTriangles();

	return count;
}

// ------------------------------------------------------------------------ //

void MeshNode::createMeshCollisionGeometries()
{
	if (numTriangles() > 0) {
#ifdef USE_INSTANCE_SCENE_GEOMETRY
		if (!m_rtcScene) {
			m_rtcScene = rtcNewScene(m_scene.rtcDevice());
			rtcSetSceneBuildQuality(m_rtcScene, RTC_BUILD_QUALITY_HIGH);

//			m_rtcScene = rtcDeviceNewScene(m_scene.rtcDevice(), RTC_SCENE_DYNAMIC, RTC_INTERSECT1);
//			m_instID = rtcNewInstance2(m_scene.rtcScene(), m_rtcScene);

			for (const auto & geom : m_geometries)
				geom->createCollisionGeometry();

			rtcCommitScene(m_rtcScene);

			m_rtcInstance = rtcNewGeometry(m_scene.rtcDevice(), RTC_GEOMETRY_TYPE_INSTANCE);
//			rtcEnableGeometry(m_rtcInstance);
			rtcSetGeometryUserData(m_rtcInstance, this);
			rtcSetGeometryInstancedScene(m_rtcInstance, m_rtcScene);
			rtcSetGeometryTimeStepCount(m_rtcInstance, 1);
			rtcSetGeometryTransform(m_rtcInstance, 0, RTC_FORMAT_FLOAT4X4_COLUMN_MAJOR, glm::value_ptr(m_matGlobal));
			rtcCommitGeometry(m_rtcInstance);

			m_instID = rtcAttachGeometry(m_scene.rtcScene(), m_rtcInstance);
			rtcCommitScene(m_scene.rtcScene());

			auto error = rtcGetDeviceError(m_scene.rtcDevice());
			if (error != RTC_ERROR_NONE) {
				qDebug() << "Embree error:" << error;
			}
		}
#else
		for (const auto & geom : m_geometries)
			geom->createCollisionGeometry();
#endif
	}
}

void MeshNode::destroyMeshCollisionGeometries()
{
#ifdef USE_INSTANCE_SCENE_GEOMETRY
	if (m_instID != RTC_INVALID_GEOMETRY_ID) {
		rtcDetachGeometry(m_scene.rtcScene(), m_instID);
		m_instID = RTC_INVALID_GEOMETRY_ID;
	}

	if (m_rtcInstance) {
		rtcReleaseGeometry(m_rtcInstance);
		m_rtcInstance = nullptr;
	}

	if (m_rtcScene) {
		for (const auto & geom : m_geometries)
			geom->destroyCollisionGeometry();

		rtcReleaseScene(m_rtcScene);
		m_rtcScene = nullptr;
	}
#else
	for (const auto & geom : m_geometries)
		geom->destroyCollisionGeometry();
#endif
}

void MeshNode::createCollisionGeometries()
{
	if (!m_visible)
		return;

	Node::createCollisionGeometries();

	createMeshCollisionGeometries();
}

void MeshNode::updateMeshCollisionGeometries()
{
	destroyMeshCollisionGeometries();
	createMeshCollisionGeometries();
}

void MeshNode::destroyCollisionGeometries()
{
	Node::destroyCollisionGeometries();

	destroyMeshCollisionGeometries();
}

// ------------------------------------------------------------------------ //

bool MeshNode::isMaterialUsed(const IMaterial * material) const
{
	for (const auto & geom : m_geometries) {
		if (geom->material() == material)
			return true;
	}

	return Node::isMaterialUsed(material);
}

void MeshNode::findGeometriesByMaterial(std::vector<IGeometry *> & geometries, const IMaterial *material) const
{
	Node::findGeometriesByMaterial(geometries, material);

	for (const auto & geom : m_geometries) {
		if (geom->material() == material)
			geometries.push_back(geom.get());
	}
}

void MeshNode::findMeshNodesByRenderLayer(std::vector<MeshNode *> & meshNodes, IRenderLayer * renderLayer)
{
	if (m_renderLayer.getValue() == renderLayer)
		meshNodes.push_back(this);

	Node::findMeshNodesByRenderLayer(meshNodes, renderLayer);
}

void MeshNode::findLights(std::vector<AbstractLight *> & lights, bool includeGeomLights)
{
	if (includeGeomLights) {
		for (const auto & geom : m_geometries) {
			if (geom->isLightSource())
				lights.push_back(geom.get());
		}
	}

	Node::findLights(lights, includeGeomLights);
}

// ------------------------------------------------------------------------ //

void MeshNode::addToAABB(BBox & bbox, bool checkVisibility) const
{
	if (checkVisibility && !m_visible.get()) return;

	Node::addToAABB(bbox, checkVisibility);

	for (const auto & geom : m_geometries)
		geom->addToAABB(bbox);
}

void MeshNode::collectBottomVertices(float threshold, vec2 & sumXY, int & count) const
{
	Node::collectBottomVertices(threshold, sumXY, count);

	for (const auto & geom : m_geometries) {
		for (const auto & v : geom->vertices()) {
			auto p = glm::transformCoord(v.pos, m_matGlobal);
			if (p.z <= threshold) {
				sumXY.x += p.x;
				sumXY.y += p.y;
				count++;
			}
		}
	}
}

void MeshNode::calculateBoundingBox()
{
	Node::calculateBoundingBox();

	m_bboxGeom.clear();
	for (const auto & geom : m_geometries)
		m_bboxGeom.addToBounds(geom->bbox());

	m_oobb.addToBounds(m_bboxGeom);
}

void MeshNode::updateVisibility(bool parentVisible)
{
	Node::updateVisibility(parentVisible);

	if (parentVisible & m_visible.get()) {
#ifdef USE_INSTANCE_SCENE_GEOMETRY
		if (m_instID == RTC_INVALID_GEOMETRY_ID && m_rtcInstance)
			m_instID = rtcAttachGeometry(m_scene.rtcScene(), m_rtcInstance);
		else
			createCollisionGeometries();
#else
		for (const auto & geom : m_geometries)
			rtcEnable(m_scene.rtcScene(), geom->geomID());
#endif
	} else {
#ifdef USE_INSTANCE_SCENE_GEOMETRY
		if (m_instID != RTC_INVALID_GEOMETRY_ID) {
			rtcDetachGeometry(m_scene.rtcScene(), m_instID);
			m_instID = RTC_INVALID_GEOMETRY_ID;
		}
#else
		for (const auto & geom : m_geometries)
			rtcDisable(m_scene.rtcScene(), geom->geomID());
#endif
	}
}

void MeshNode::updateTransformation()
{
	Node::updateTransformation();

#ifdef USE_INSTANCE_SCENE_GEOMETRY
	if (m_instID != RTC_INVALID_GEOMETRY_ID) {
		rtcSetGeometryTransform(m_rtcInstance, 0, RTC_FORMAT_FLOAT4X4_COLUMN_MAJOR, glm::value_ptr(m_matGlobal));
		rtcCommitGeometry(m_rtcInstance);
//		rtcCommitScene(m_scene.rtcScene());
	}
#else
	for (const auto & geom : m_geometries)
		geom->UpdateTransformation(m_scene.rtcScene(), m_matGlobal);
#endif

	for (const auto & geom : m_geometries)
		geom->resetArea();
}
