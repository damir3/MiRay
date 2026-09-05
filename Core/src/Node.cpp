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

#include "Node.h"

#include "../../Shared/Interfaces/ModelLoader.h"

#define MIN_POSITION	-1e+5f	// -1000.00m
#define MAX_POSITION	+1e+5f	// 1000.00m
#define MIN_SCALE		1e-3f
#define MAX_SCALE		1e+3f

// ------------------------------------------------------------------------ //

Node::Node(const QString & name, Scene & scene, Node * parent)
	: m_scene(scene)
	, m_guid(scene.createGuid())
	, m_parent(parent)
	, m_matLocal(1.f)
	, m_matGlobal(parent ? parent->m_matGlobal : mat4(1.f))
	, m_matInvGlobal(parent ? parent->m_matInvGlobal : mat4(1.f))
	, m_parentVisible(parent ? parent->m_parentVisible : true)
	, m_uniqueColor(scene.generateUniqueColor())
	, m_name(name, PID_NODE_NAME, *this, nullptr)
	, m_visible(true, PID_NODE_VISIBLE, *this)
	, m_position(vec3(0.f), 2, PID_NODE_POSITION, *this)
	, m_rotation(vec3(0.f), 1, PID_NODE_ROTATION, *this)
	, m_scale(vec3(1.f), 3, PID_NODE_SCALE, *this)
{
	m_oobb.clear();
}

Node::~Node()
{
}

// ------------------------------------------------------------------------ //

void Node::removeChildren()
{
	m_children.clear();
}

// ------------------------------------------------------------------------ //

void Node::setParent(Node * parent)
{
	m_parent = parent;
}

bool Node::hasParent(INode * parent) const
{
	auto node = m_parent;
	while (node) {
		if (node == parent)
			return true;

		node = node->m_parent;
	}

	return false;
}

QJsonObject Node::getState(bool fullInfo)
{
	bool flipFacing = glm::dot(glm::cross(glm::axisX(m_matGlobal), glm::axisY(m_matGlobal)), glm::axisZ(m_matGlobal)) < 0.f;
	return QJsonObject {
		{ "f", (m_visible.get() ? 1 : 0) | (flipFacing ? 2 : 0) },
		{ "p", toJsonArray(m_position.get()) },
		{ "r", toJsonArray(m_rotation.get()) },
		{ "s", toJsonArray(m_scale.get()) }
	};
}

void Node::setState(const QJsonObject & state, uint32_t snapshotFlags)
{
	if (snapshotFlags & SF_HAS_VISIBILITY) {
		int flags = state.value("f").toInt(m_visible.get() ? 1 : 0);
		bool visible = flags & 1;
		if (m_visible.get() != visible) {
			m_visible._set(visible);
			m_scene.fireNodeChanged(this, NodeChanged_Visibility);
		}
	}

	if (snapshotFlags & SF_HAS_TRANSFORMATIONS) {
		_setTransformation(getVec3(state, "p", m_position.get()), getVec3(state, "r", m_rotation.get()), getVec3(state, "s", m_scale.get()));
		m_scene.fireNodeChanged(this, NodeChanged_Transformation);
	}
}

void Node::getStateRecursive(QJsonObject & nodes, QJsonObject & geoms, bool fullInfo)
{
	if (m_parent)
		nodes[m_guid] = getState(fullInfo);

	for (const auto & node : m_children)
		node->getStateRecursive(nodes, geoms, fullInfo);
}

void Node::setStateRecursive(const QJsonObject & nodes, const QJsonObject & geoms, uint32_t flags)
{
	if (nodes.contains(m_guid))
		setState(nodes[m_guid].toObject(), flags);

	for (const auto & node : m_children)
		node->setStateRecursive(nodes, geoms, flags);

	if (!m_parent && (flags & SF_HAS_VISIBILITY)) // update visibility from root node
		updateVisibility(true);
}

void Node::setAnimationStateRecursive(const QJsonObject & nodes1, const QJsonObject & nodes2, const QJsonObject & geoms1, const QJsonObject & geoms2, uint32_t flags)
{
	m_animState[0].visible = m_animState[1].visible = m_visible.get();
	m_animState[0].position = m_animState[1].position = m_position.get();
	m_animState[0].rotation = m_animState[1].rotation = m_rotation.get();
	m_animState[0].scale = m_animState[1].scale = m_scale.get();

	if (nodes1.contains(m_guid) && nodes2.contains(m_guid)) {
		QJsonObject state1 = nodes1[m_guid].toObject();
		QJsonObject state2 = nodes2[m_guid].toObject();
		if (flags & SF_HAS_VISIBILITY) {
			m_animState[0].visible = state1.value("f").toInt(m_animState[0].visible ? 1 : 0) & 1;
			m_animState[1].visible = state2.value("f").toInt(m_animState[1].visible ? 1 : 0) & 1;
		}

		if (flags & SF_HAS_TRANSFORMATIONS) {
			m_animState[0].position = getVec3(state1, "p", m_animState[0].position);
			m_animState[1].position = getVec3(state2, "p", m_animState[1].position);
			m_animState[0].rotation = getVec3(state1, "r", m_animState[0].rotation);
			m_animState[1].rotation = getVec3(state2, "r", m_animState[1].rotation);
			m_animState[0].scale = getVec3(state1, "s", m_animState[0].scale);
			m_animState[1].scale = getVec3(state2, "s", m_animState[1].scale);
		}
	}

	for (const auto & node : m_children)
		node->setAnimationStateRecursive(nodes1, nodes2, geoms1, geoms2, flags);
}

void Node::setAnimationTimeRecursive(float t, Sampler &sampler)
{
	m_visible._set(m_animState[sampler.generate1D() > t ? 0 : 1].visible);

	auto position = glm::mix(m_animState[0].position, m_animState[1].position, t);
	auto rotation = glm::mix(m_animState[0].rotation, m_animState[1].rotation, t);
	auto scale = glm::mix(m_animState[0].scale, m_animState[1].scale, t);
	m_matLocal = MatrixBuilder().AddPosition(position).AddRotation(rotation).AddScale(scale);

	for (const auto & node : m_children)
		node->setAnimationTimeRecursive(t, sampler);

	if (!m_parent) { // recursively update visibility and transformation from the root node
		updateVisibility(true);
		updateTransformation();
	}
}

void Node::_setVisible(bool visible)
{
	m_visible._set(visible);
	if (m_parentVisible)
		updateVisibility(m_parentVisible);
	m_scene.fireNodeChanged(this, NodeChanged_Visibility);
}

void Node::_setTransformation(const vec3 & position, const vec3 & rotation, const vec3 & scale)
{
	m_position._set(position);
	m_rotation._set(rotation);
	m_scale._set(scale);
	onTransformationParamChanged();
}

void Node::_setTransformation(const mat4 & matLocal)
{
	m_matLocal = matLocal;
	updateTransformation();
	onBoundingBoxChanged();

	m_scene.fireNodeChanged(this, NodeChanged_Transformation);
}

void Node::onTransformationParamChanged()
{
	auto p = m_position.get();
	auto r = m_rotation.get();
	auto s = m_scale.get();
	p.x = std::clamp(p.x, MIN_POSITION, MAX_POSITION);
	p.y = std::clamp(p.y, MIN_POSITION, MAX_POSITION);
	p.z = std::clamp(p.z, MIN_POSITION, MAX_POSITION);
	s.x = std::clamp(s.x, -MAX_SCALE, MAX_SCALE);
	s.y = std::clamp(s.y, -MAX_SCALE, MAX_SCALE);
	s.z = std::clamp(s.z, -MAX_SCALE, MAX_SCALE);
	if (std::fabs(s.x) < MIN_SCALE) s.x = s.x < 0.f ? -MIN_SCALE : MIN_SCALE;
	if (std::fabs(s.y) < MIN_SCALE) s.y = s.y < 0.f ? -MIN_SCALE : MIN_SCALE;
	if (std::fabs(s.z) < MIN_SCALE) s.z = s.z < 0.f ? -MIN_SCALE : MIN_SCALE;
	_setTransformation(MatrixBuilder().AddPosition(p).AddRotation(r).AddScale(s));
}

void Node::setTransformation(const mat4 & matLocal)
{
	m_matLocal = matLocal;
	updateTransformation();
	m_position._set(glm::translation(m_matLocal));
	m_rotation._set(glm::getRotation(m_matLocal));
	m_scale._set(glm::getScale(m_matLocal));

	onBoundingBoxChanged();

	m_scene.fireNodeChanged(this, NodeChanged_Transformation);
}

void Node::setGlobalTransformation(const mat4 & matGlobal)
{
	setTransformation(glm::inverse(m_parent ? m_parent->globalTransformation() : mat4(1.f)) * matGlobal);
}

// ------------------------------------------------------------------------ //

INode *Node::addChildNode(const QString &name, eSceneElementType type)
{
	Node * node = nullptr;
	switch (type) {
		case SceneElement_Node:				node =	new Node(name, m_scene, this); break;
		case SceneElement_MeshNode:			node =	new MeshNode(name, m_scene, this); break;
		case SceneElement_Light:			node = new LightNode(name, m_scene, this); break;
		case SceneElement_DirectionalLight:	node = new DirectionalLight(name, m_scene, this); break;
	}

	if (node) {
		m_children.emplace_back(node);
	}

	return node;
}

NodePtr Node::removeChildNode(size_t pos)
{
	assert(pos < m_children.size());
	m_children[pos]->updateVisibility(false);

	NodePtr removedNode = std::move(m_children[pos]);
	m_children.erase(m_children.begin() + pos);

	m_scene.fireNodeChanged(this, NodeChanged_ChildrenList);

	onBoundingBoxChanged();
	return removedNode;
}

void Node::insertChildNode(NodePtr & node, int pos)
{
	assert(node);
	node->setParent(this);
	node->updateVisibility(m_parentVisible & m_visible.get());

	if (pos < 0 || pos >= static_cast<int>(m_children.size())) {
		m_children.push_back(std::move(node));
	} else {
		m_children.insert(m_children.begin() + pos, std::move(node));
	}

	m_scene.fireNodeChanged(this, NodeChanged_ChildrenList);

	onBoundingBoxChanged();
}

int Node::getChildIndex(const Node * child) const
{
	auto it = std::find_if(m_children.begin(), m_children.end(), [&](const NodePtr & item) {
		return item.get() == child;
	});
	if (it != m_children.end()) {
		return static_cast<int>(std::distance(m_children.begin(), it));
	}
	return -1;
}

void Node::pasteNodes(const QByteArray & data)
{
	m_scene.lock(SceneInternalModification_Import);

	auto undoCommand = new SceneImportCommand(m_scene, *this);

	auto loaders = m_scene.context().modelLoadersRegistry();
	QSharedPointer<IModelLoader> loader(loaders->getLoader("data.mirayScene").release());

	INodeImporter *importer = qobject_cast<INodeImporter *>(loader.data());
	assert(importer);
	importer->importNodes(this, &m_scene, data);

	createCollisionGeometries();
	onBoundingBoxChanged();

	m_scene.fireNodeChanged(this, NodeChanged_ChildrenList);
	m_scene.materialManager().fireMaterialListChanged();

	m_scene.pushCommand(undoCommand);

	m_scene.unlock(SceneInternalModification_Import);
}

size_t Node::numChildren(eSceneElementType type) const
{
	return (type & SceneElement_Node) ? m_children.size() : 0;
}

ISceneElement * Node::child(size_t i, eSceneElementType type) const
{
	if (type & SceneElement_Node) {
		assert(i < m_children.size());
		return m_children[i].get();
	}

	return nullptr;

}

bool Node::hasChild(INode * node) const
{
	for (const auto & childNode : m_children) {
		if (childNode.get() == node || childNode->hasChild(node))
			return true;
	}

	return false;
}

// ------------------------------------------------------------------------ //

void Node::createCollisionGeometries()
{
	if (!m_visible)
		return;

	for (const auto & child : m_children)
		child->createCollisionGeometries();
}

void Node::destroyCollisionGeometries()
{
	for (const auto & child : m_children)
		child->destroyCollisionGeometries();
}

// ------------------------------------------------------------------------ //

bool Node::isMaterialUsed(const IMaterial * material) const
{
	for (const auto & child : m_children) {
		if (child->isMaterialUsed(material))
			return true;
	}

	return false;
}

void Node::findGeometriesByMaterial(std::vector<IGeometry *> & geometries, const IMaterial * material) const
{
	for (const auto & child : m_children)
		child->findGeometriesByMaterial(geometries, material);
}

void Node::findMeshNodesByRenderLayer(std::vector<MeshNode *> & meshNodes, IRenderLayer * renderLayer)
{
	for (const auto & child : m_children)
		child->findMeshNodesByRenderLayer(meshNodes, renderLayer);
}

void Node::findLights(std::vector<AbstractLight *> & lights, bool includeGeomLights)
{
	for (const auto & child : m_children) {
		if (child->visible().get())
			child->findLights(lights, includeGeomLights);
	}
}

// ------------------------------------------------------------------------ //

void Node::addToAABB(BBox & bbox, bool checkVisibility) const
{
	if (checkVisibility && !m_visible.get()) return;

	for (const auto & child : m_children)
		child->addToAABB(bbox, checkVisibility);
}

BBox Node::getAABB(bool checkVisibility) const
{
	BBox bbox;
	bbox.clear();
	addToAABB(bbox, checkVisibility);
	if (bbox.isEmpty())
		bbox.addToBounds(glm::translation(m_matGlobal));
	return bbox;
}

void Node::collectBottomVertices(float threshold, vec2 & sumXY, int & count) const
{
	for (const auto & child : m_children)
		child->collectBottomVertices(threshold, sumXY, count);
}

vec3 Node::getObjectBottomPoint(const BBox & bbox) const
{
	float minZ = bbox.min.z;
	float threshold = minZ + 1e-3f;
	vec2 sumXY(0.f);
	int count = 0;

	collectBottomVertices(threshold, sumXY, count);

	if (count > 0)
		return vec3(sumXY.x / count, sumXY.y / count, minZ);

	return vec3(bbox.center().x, bbox.center().y, minZ);
}

vec3 Node::getObjectBottomPoint() const
{
	return getObjectBottomPoint(getAABB());
}

void Node::calculateBoundingBox()
{
	m_oobb.clear();
	for (const auto & child : m_children) {
		BBox childBBox = child->m_oobb;
		if (!childBBox.isNull()) {
			childBBox.transform(child->m_matLocal);
			m_oobb.addToBounds(childBBox);
		}
	}
}

void Node::onBoundingBoxChanged()
{
	auto node = this;
	while (node) {
		node->calculateBoundingBox();
		node = node->m_parent;
	}
}

void Node::updateBoundingBox()
{
	for (const auto & child : m_children)
		child->updateBoundingBox();

	calculateBoundingBox();
}

void Node::updateVisibility(bool parentVisible)
{
	m_parentVisible = parentVisible;
	parentVisible &= m_visible.get();
	for (const auto & child : m_children)
		child->updateVisibility(parentVisible);
}

void Node::updateTransformation()
{
	m_matGlobal = m_parent ? m_parent->m_matGlobal * m_matLocal : m_matLocal;
	m_matInvGlobal = glm::inverse(m_matGlobal);
	for (const auto & child : m_children)
		child->updateTransformation();
}

// ------------------------------------------------------------------------ //

CoreInstance & Node::core() const
{
	return m_scene.core();
}

void Node::pushCommand(QUndoCommand *cmd)
{
	m_scene.pushCommand(cmd);
}

void Node::fireChanged(eParamId paramId)
{
	switch (paramId) {
		case PID_NODE_NAME:
			m_scene.fireNodeChanged(this, NodeChanged_Name);
			break;

		case PID_NODE_VISIBLE: {
			m_scene.lock(SceneInternalModification_Geometries);
			if (m_parentVisible)
				updateVisibility(m_parentVisible);
			m_scene.fireNodeChanged(this, NodeChanged_Visibility);
			m_scene.unlock(SceneInternalModification_Geometries);
			break;
		}

		case PID_NODE_POSITION:
		case PID_NODE_ROTATION:
		case PID_NODE_SCALE: {
			m_scene.lock(SceneInternalModification_Transformation);
			onTransformationParamChanged();
			m_scene.unlock(SceneInternalModification_Transformation);
			break;
		}
	}
}
