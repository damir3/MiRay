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

#include "SceneCommands.h"
#include "Geometry.h"

#include "NodeUtils.h"

namespace
{
class CommandDelete_DeleteNode : public QUndoCommand
{
	Scene & m_scene;
	Node *  m_parent;
	Node *  m_nodeRaw;
	NodePtr m_nodePtr;
	int     m_pos;
public:
	CommandDelete_DeleteNode(Node *node, QUndoCommand *parentCmd, Scene &scene)
		: QUndoCommand(parentCmd)
		, m_scene(scene)
		, m_nodeRaw(node)
		, m_parent(static_cast<Node *>(node->parent()))
		, m_pos(-1)
	{
		assert(m_nodeRaw);
		assert(m_parent);
	}

	void redo()
	{
		assert(m_nodeRaw->numChildren(SceneElement_Node) == 0); // can't delete non-empty nodes

		m_pos = m_parent->getChildIndex(m_nodeRaw);
		auto mod = sceneModificationDelete(m_nodeRaw, m_pos);
		m_scene.fireBeforeSceneUpdate(mod);

		m_nodePtr = m_parent->removeChildNode(static_cast<size_t>(m_pos));

		m_scene.fireAfterSceneUpdate(mod);
	}

	void undo()
	{
		auto mod = sceneModificationAdd(m_nodeRaw, m_parent, m_pos);
		m_scene.fireBeforeSceneUpdate(mod);

		m_parent->insertChildNode(m_nodePtr, m_pos);

		m_scene.fireAfterSceneUpdate(mod);
	}
};

class CommandDelete_DeleteGeometry : public QUndoCommand
{
	Scene &     m_scene;
	GeometryPtr m_geometry;
	Geometry *  m_geometryRaw;
	MeshNode *  m_node;
	int         m_pos;

public:
	CommandDelete_DeleteGeometry(IGeometry *geometry, QUndoCommand *parentCmd, Scene &scene)
		: QUndoCommand(parentCmd)
		, m_scene(scene)
		, m_geometry(nullptr)
		, m_geometryRaw(dynamic_cast<Geometry *>(geometry))
		, m_node(m_geometryRaw->meshNode())
		, m_pos(-1)
	{
	}

	void redo()
	{
		m_pos = m_node->getGeometryIndex(m_geometryRaw);
		auto mod = sceneModificationDelete(m_geometryRaw, m_pos);
		m_scene.fireBeforeSceneUpdate(mod);

		m_geometry = m_node->removeGeometry(static_cast<size_t>(m_pos));

		m_scene.fireAfterSceneUpdate(mod);
	}

	void undo()
	{
		auto mod = sceneModificationAdd(m_geometryRaw, m_node, m_pos);
		m_scene.fireBeforeSceneUpdate(mod);

		m_node->insertGeometry(m_geometry, m_pos);

		m_scene.fireAfterSceneUpdate(mod);
	}
};

class CommandCreateNode : public QUndoCommand
{
	Scene &   m_scene;
	Node *    m_nodeRaw;
	NodePtr   m_nodePtr;
	Selection m_selection;

public:
	CommandCreateNode(Scene & scene, NodePtr & node)
		: m_scene(scene)
		, m_nodeRaw(node.get())
		, m_nodePtr(std::move(node))
		, m_selection(scene.selection())
	{
	}

	void redo()
	{
		m_scene.lock(SceneInternalModification_Geometries);

		auto &root = m_scene.root();
		auto pos = static_cast<int>(root.children().size());

		auto mod = sceneModificationAdd(m_nodeRaw, &root, pos);
		m_scene.fireBeforeSceneUpdate(mod);

		root.insertChildNode(m_nodePtr, pos);

		m_scene.fireAfterSceneUpdate(mod);

		Selection selection;
		selection.insert(m_nodeRaw);
		m_scene.setSelection(selection, SelectionOperation_Set);

		m_scene.unlock(SceneInternalModification_Geometries);
	}

	void undo()
	{
		m_scene.lock(SceneInternalModification_Geometries);

		auto &root = m_scene.root();

		int idx = root.getChildIndex(m_nodeRaw);
		assert(idx >= 0);
		auto mod = sceneModificationDelete(m_nodeRaw, idx);

		m_scene.fireBeforeSceneUpdate(mod);

		m_nodePtr = root.removeChildNode(static_cast<size_t>(idx));

		m_scene.fireAfterSceneUpdate(mod);

		m_scene.setSelection(m_selection, SelectionOperation_Set);

		m_scene.unlock(SceneInternalModification_Geometries);
	}
};

class CommandDeleteElements : public QUndoCommand
{
	Scene	&m_scene;
	const Selection m_initialSelection;

public:

	CommandDeleteElements(Scene &scene, const Selection &elements)
		: m_scene(scene)
		, m_initialSelection(elements)
	{
		// separate nodes and meshes
		std::vector<IGeometry *> meshes;
		std::vector<INode *> nodes;
		for (auto e : elements) {
			if (!e) continue;

			if (e->type() == SceneElement_Geometry) {
				auto g = static_cast<IGeometry *>(e);
				auto parentNode = static_cast<Node *>(g->parent());
				if (&parentNode->scene() == &scene)
					meshes.push_back(g);
			} else {
				auto n = static_cast<Node *>(e);
				if (&n->scene() == &scene)
					nodes.push_back(n);
			}
		}

		// first delete meshes
		for (auto mesh : meshes)
			new CommandDelete_DeleteGeometry(mesh, this, scene);

		// then delete nodes
		auto v = collectAllChildren(nodes);
		v = sortAccordingToTheSceneTree(v, &scene.root(), true);
		for (auto node : v)
			new CommandDelete_DeleteNode(static_cast<Node *>(node), this, scene);
	}

	void redo()
	{
		m_scene.lock(SceneInternalModification_Geometries);
		QUndoCommand::redo();
		m_scene.setSelection(Selection(), SelectionOperation_Set);
		m_scene.unlock(SceneInternalModification_Geometries);
	}

	void undo()
	{
		m_scene.lock(SceneInternalModification_Geometries);
		QUndoCommand::undo();
		m_scene.setSelection(m_initialSelection, SelectionOperation_Set);
		m_scene.unlock(SceneInternalModification_Geometries);
	}
};

class CommandMoveNodes : public QUndoCommand
{
	Scene	&m_scene;
	Node	*m_targetNode;
	NodePtr	m_groupNode;
	Node	*m_groupNodeRaw;
	const Node	*m_nodeBefore;
	bool	m_undo;
	struct NodeInfo {
		Node	*node;
		Node	*oldParent;
		size_t	oldPos;
		mat4	matLocalOld;

		NodeInfo(Node *n) : node(n), oldParent(n->parent()), oldPos(0), matLocalOld(n->transformation()) {}
	};
	std::vector<NodeInfo> m_nodes;

public:
	CommandMoveNodes(Scene &scene, Node *target, size_t pos, const QString &groupName, const NodeSelection &nodes)
		: m_scene(scene)
		, m_targetNode(target)
		, m_groupNode(nullptr)
		, m_groupNodeRaw(nullptr)
		, m_undo(true)
	{
		if (&target->scene() != &scene)
			return;

		for (auto node : nodes) {
			if (&static_cast<Node *>(node)->scene() != &scene)
				return;
		}

		m_nodeBefore = pos < target->numChildren(SceneElement_Node) ? static_cast<Node *>(target->child(pos, SceneElement_Node)) : nullptr;
		if (!groupName.isEmpty()) {
			m_groupNode = std::make_unique<Node>(groupName, m_scene, nullptr);
			m_groupNodeRaw = m_groupNode.get();
		}

		m_nodes.reserve(nodes.size());
		for (auto node : nodes) {
			if (node != target && !target->hasParent(node) &&
				!(node == m_nodeBefore && m_groupNode == nullptr)) {
				auto it = std::find_if(m_nodes.begin(), m_nodes.end(), [node](const NodeInfo & ni)->bool { return ni.node == node; });
				if (it == m_nodes.end())
					m_nodes.emplace_back(NodeInfo(static_cast<Node *>(node)));
			}
		}
	}

	void redo()
	{
		m_scene.lock(SceneInternalModification_Import);

		Node *target = m_targetNode;
		size_t targetPos = target->children().size();
		if (m_nodeBefore) {
			int idx = target->getChildIndex(m_nodeBefore);
			assert(idx >= 0);
			targetPos = idx;
		}

		if (m_groupNodeRaw) {
			target->insertChildNode(m_groupNode, static_cast<int>(targetPos));
			m_groupNodeRaw->setTransformation(mat4(1.f));
			target = m_groupNodeRaw;
		}

		for (auto & ni : m_nodes) {
			const auto matGlobal = ni.node->globalTransformation();

			int idx = ni.oldParent->getChildIndex(ni.node);
			assert(idx >= 0);
			ni.oldPos = idx;

			NodePtr movedNode = ni.oldParent->removeChildNode(ni.oldPos);

			size_t insertPos;
			if (m_nodeBefore && !m_groupNodeRaw) {
				int idx = target->getChildIndex(m_nodeBefore);
				assert(idx >= 0);
				insertPos = idx;
			} else {
				insertPos = target->children().size();
			}

			target->insertChildNode(movedNode, static_cast<int>(insertPos));
			ni.node->setTransformation(target->inverseGlobalTransformation() * matGlobal);
		}

		m_scene.unlock(SceneInternalModification_Import);
		m_undo = false;
	}

	void undo()
	{
		m_scene.lock(SceneInternalModification_Import);

		Node *target = m_groupNodeRaw ? m_groupNodeRaw : m_targetNode;
		for (auto it = m_nodes.rbegin(); it != m_nodes.rend(); ++it) {
			int idx = target->getChildIndex(it->node);
			assert(idx >= 0);
			size_t curPos = idx;

			NodePtr movedNode = target->removeChildNode(curPos);
			it->oldParent->insertChildNode(movedNode, static_cast<int>(it->oldPos));
			it->node->setTransformation(it->matLocalOld);
		}

		if (m_groupNodeRaw) {
			int idx = m_targetNode->getChildIndex(m_groupNodeRaw);
			assert(idx >= 0);
			size_t groupPos = idx;

			m_groupNode = m_targetNode->removeChildNode(groupPos);
		}

		m_scene.unlock(SceneInternalModification_Import);
		m_undo = true;
	}
};

class VisibilityCommand : public QUndoCommand
{
	Scene	&m_scene;
	struct AffectedNode {
		Node	*node;
		bool	oldVisible, newVisible;
	};
	std::vector<AffectedNode>	m_nodes;

	void redo()
	{
		m_scene.lock(SceneInternalModification_Geometries);

		for (auto n : m_nodes) {
			auto mod = sceneModificationVisibility(n.node);
			m_scene.fireBeforeSceneUpdate(mod);
			n.node->_setVisible(n.newVisible);
			m_scene.fireAfterSceneUpdate(mod);
		}

		m_scene.unlock(SceneInternalModification_Geometries);
	}

	void undo()
	{
		m_scene.lock(SceneInternalModification_Geometries);

		for (auto n : m_nodes) {
			auto mod = sceneModificationVisibility(n.node);
			m_scene.fireBeforeSceneUpdate(mod);
			n.node->_setVisible(n.oldVisible);
			m_scene.fireAfterSceneUpdate(mod);
		}

		m_scene.unlock(SceneInternalModification_Geometries);
	}

public:
	VisibilityCommand(Scene &scene, const NodeSelection &v, bool visible)
		: m_scene(scene)
	{
		for (auto p : v) {
			AffectedNode node;
			node.node = static_cast<Node *>(p);
			node.oldVisible = p->visible().get();
			node.newVisible = visible;
			m_nodes.push_back(node);
		}
	}
};

class MaterialCommand : public QUndoCommand
{
	struct AffectedGeometry {
		IGeometry *	geometry;
		IMaterial *	material;
	};

	Scene &	m_scene;
	std::vector<AffectedGeometry>	m_geometries;
	IMaterial *	m_newMaterial;

	void redo()
	{
		m_scene.lock(SceneInternalModification_Material);

		for (auto & g : m_geometries) {
			auto mod = sceneModificationMaterial(g.geometry);
			m_scene.fireBeforeSceneUpdate(mod);
			static_cast<Geometry *>(g.geometry)->setMaterial(m_newMaterial);
			m_scene.fireAfterSceneUpdate(mod);
		}

		m_scene.unlock(SceneInternalModification_Material);
	}

	void undo()
	{
		m_scene.lock(SceneInternalModification_Material);

		for (auto & g : m_geometries) {
			auto mod = sceneModificationMaterial(g.geometry);
			m_scene.fireBeforeSceneUpdate(mod);
			static_cast<Geometry *>(g.geometry)->setMaterial(g.material);
			m_scene.fireAfterSceneUpdate(mod);
		}

		m_scene.unlock(SceneInternalModification_Material);
	}

public:
	MaterialCommand(Scene & scene, const std::vector<IGeometry *> & geometries, IMaterial * material)
		: m_scene(scene)
		, m_newMaterial(material)
	{
		m_geometries.reserve(geometries.size());
		for (auto geom : geometries) {
			AffectedGeometry affectedGeom = {geom, geom->material()};
			m_geometries.push_back(affectedGeom);
		}
	}
};

template <typename ActiveContainer, typename OldSet, typename NewContainer>
void extractNewElements(ActiveContainer & activeElements, const OldSet & oldElements, NewContainer & newElements)
{
	for (auto it = activeElements.begin(); it != activeElements.end(); ) {
		if (!oldElements.contains(it->get())) {
			newElements.push_back(std::move(*it));
			it = activeElements.erase(it);
		} else {
			++it;
		}
	}
}

template <typename ActiveContainer, typename NewContainer>
void restoreNewElements(ActiveContainer & activeElements, NewContainer & newElements)
{
	activeElements.reserve(activeElements.size() + newElements.size());
	for (auto & element : newElements) {
		activeElements.push_back(std::move(element));
	}
	newElements.clear();
}

} // namespace

SceneImportCommand::SceneImportCommand(Scene & scene, Node & root)
	: m_scene(scene)
	, m_root(root)
	, m_undo(true)
{
	for (const auto & child : m_root.children()) {
		m_oldNodes.insert(child.get());
	}
	for (const auto &material : m_scene.materialManager().materials()) {
		m_oldMaterials.insert(material.get());
	}
	for (const auto &snap : m_scene.snapshotManager().snapshots()) {
		m_oldSnapshots.insert(snap.get());
	}
}

void SceneImportCommand::toggleSceneState()
{
	m_scene.lock(SceneInternalModification_Import);

	m_root.destroyCollisionGeometries();
	if (m_undo) {
		extractNewElements(m_root.children(), m_oldNodes, m_newNodes);
	} else {
		restoreNewElements(m_root.children(), m_newNodes);
	}

	m_scene.buildCollisionScene();
	m_scene.fireNodeChanged(&m_root, NodeChanged_ChildrenList);

	auto & activeMaterials = m_scene.materialManager().materials();
	if (m_undo) {
		extractNewElements(activeMaterials, m_oldMaterials, m_newMaterials);
	} else {
		restoreNewElements(activeMaterials, m_newMaterials);
	}
	m_scene.materialManager().fireMaterialListChanged();

	auto & activeSnapshots = m_scene.snapshotManager().snapshots();
	if (m_undo) {
		extractNewElements(activeSnapshots, m_oldSnapshots, m_newSnapshots);
	} else { // redo
		restoreNewElements(activeSnapshots, m_newSnapshots);
	}
	m_scene.snapshotManager().fireChanged();

	m_scene.unlock(SceneInternalModification_Import);

	m_scene.setSelection(m_undo ? Selection() : Selection(m_newNodesRaw.begin(), m_newNodesRaw.end()), SelectionOperation_Set);

	m_undo = !m_undo;
}

void SceneImportCommand::redo()
{
	if (!m_undo) {
		toggleSceneState();
	} else {// first redo
		assert(m_newNodesRaw.empty());
		assert(m_newMaterials.empty());
		assert(m_newSnapshots.empty());

		for (const auto & node : m_root.children()) {
			if (!m_oldNodes.contains(node.get()))
				m_newNodesRaw.push_back(node.get());
		}

		m_scene.setSelection(Selection(m_newNodesRaw.begin(), m_newNodesRaw.end()), SelectionOperation_Set);
	}
}

void SceneImportCommand::undo()
{
	if (m_undo)
		toggleSceneState();
}

// ------------------------------------------------------------------------ //

void sceneShowNodes(Scene &scene, const NodeSelection &nodes, bool visible)
{
	scene.pushCommand(new VisibilityCommand(scene, nodes, visible));
}

void sceneCreateNode(Scene & s, NodePtr & node)
{
	s.pushCommand(new CommandCreateNode(s, node));
}

void sceneDeleteElements(Scene &scene, const Selection &elements)
{
	scene.pushCommand(new CommandDeleteElements(scene, elements));
}

void sceneMoveNodes(Scene &scene, Node *target, size_t pos, const QString &groupName, const NodeSelection &nodes)
{
	scene.pushCommand(new CommandMoveNodes(scene, target, pos, groupName, nodes));
}

void sceneSetMaterial(Scene &scene, const GeomSelection & geometries, IMaterial * material)
{
	scene.pushCommand(new MaterialCommand(scene, geometries, material));
}

// ------------------------------------------------------------------------ //

static bool checkParent(Node * node, const NodeSelection & nodes)
{
	for (auto * checkNode : nodes) {
		if (node != checkNode && node->hasParent(checkNode))
			return true;
	}

	return false;
}

TransformationCommand::TransformationCommand(Scene & scene, const NodeSelection & nodes, bool skipFirstRedo)
	: m_scene(scene), m_skipFirstRedo(skipFirstRedo)
{
	for (auto * node : nodes) {
		if (!checkParent(static_cast<Node *>(node), nodes))
			m_nodes.emplace_back(node, node->transformation(), node->globalTransformation());
	}
}

void TransformationCommand::setTransformation(const mat4 & mat)
{
	m_scene.lock(SceneInternalModification_Transformation);

	for (auto & node : m_nodes) {
		node.node->setGlobalTransformation(mat * node.oldGlobalTransform);
		node.newTransform = node.node->transformation();
	}

	m_scene.unlock(SceneInternalModification_Transformation);
}

void TransformationCommand::setNodeTransformation(INode * targetNode, const mat4 & mat)
{
	m_scene.lock(SceneInternalModification_Transformation);

	for (auto & node : m_nodes) {
		if (node.node == targetNode) {
			node.node->setGlobalTransformation(mat * node.oldGlobalTransform);
			node.newTransform = node.node->transformation();
			break;
		}
	}

	m_scene.unlock(SceneInternalModification_Transformation);
}

void TransformationCommand::redo()
{
	if (m_skipFirstRedo) {
		m_skipFirstRedo = false;
		return;
	}

	m_scene.lock(SceneInternalModification_Transformation);

	for (auto & n : m_nodes)
		n.node->setTransformation(n.newTransform);

	m_scene.unlock(SceneInternalModification_Transformation);
}

void TransformationCommand::undo()
{
	m_scene.lock(SceneInternalModification_Transformation);

	for (auto & n : m_nodes)
		n.node->setTransformation(n.oldTransform);

	m_scene.unlock(SceneInternalModification_Transformation);
}

// ------------------------------------------------------------------------ //

MoveMeshesCommand::MoveMeshesCommand(const GeomSelection &geoms, bool deleteEmptyNodes, Node * targetNode)
	: m_deleteEmptyNodes(deleteEmptyNodes)
	, m_parentNode(targetNode)
{
	assert(targetNode);
	m_oldSelection = targetNode->scene().selection();

	m_geoms.reserve(geoms.size());
	for (auto * geom : geoms) {
		assert(geom);
		auto * parent = static_cast<MeshNode *>(geom->parent());
		assert(parent);
		if (parent == m_parentNode) continue;
		m_geoms.emplace_back(GeomInfo(static_cast<Geometry *>(geom), parent));
	}
}

void MoveMeshesCommand::GeomInfo::transformGeomVertices(const mat4 & from, const mat4 & to) const
{
	auto matP = from * glm::inverse(to);
	auto matN = glm::transpose(glm::inverse(matP));

	auto newVertices = vertices;
	for (auto & v : newVertices) {
		v.pos = glm::transformCoord(v.pos, matP);
		v.normal = glm::normalize(glm::transformNormal(v.normal, matN));
	}

	geom->setVertices(newVertices);
}

void MoveMeshesCommand::redo()
{
	auto &scene = m_parentNode->scene();
	scene.lock(SceneInternalModification_Geometries);

	if (!m_targetNode) {
		if (m_parentNode->isMeshNode()) {
			m_targetNode = qobject_cast<MeshNode *>(m_parentNode);
		} else {
			m_targetNode = qobject_cast<MeshNode *>(m_parentNode->addChildNode("Extracted Meshes", SceneElement_MeshNode));
			scene.fireNodeChanged(m_parentNode, NodeChanged_ChildrenList);
		}
	} else if (m_targetNode != m_parentNode) {
		auto pos = static_cast<int>(m_parentNode->children().size());
		auto modAdd = sceneModificationAdd(m_targetNode, m_parentNode, pos);
		scene.fireBeforeSceneUpdate(modAdd);

		m_parentNode->insertChildNode(m_targetNodePtr, pos);

		scene.fireAfterSceneUpdate(modAdd);
	}

	Selection selection = { m_targetNode };

	for (auto & info : m_geoms) {
		selection.insert(info.geom);
		info.pos = info.parent->getGeometryIndex(info.geom);

		auto modMove = sceneModificationMove(info.geom, m_targetNode, info.pos);
		scene.fireBeforeSceneUpdate(modMove);

		GeometryPtr geomPtr = info.parent->removeGeometry(info.pos);

		info.transformGeomVertices(info.parent->globalTransformation(), m_targetNode->globalTransformation());
		m_targetNode->insertGeometry(geomPtr, static_cast<int>(m_targetNode->numGeometries()));
		if (!m_targetNode->rtcScene())
			m_targetNode->createCollisionGeometries();

		scene.fireAfterSceneUpdate(modMove);
	}

	if (m_deleteEmptyNodes) {
		std::set<MeshNode *> emptyNodes;
		for (const auto & info : m_geoms) {
			if (!info.parent->numGeometries() && info.parent->parent())
				emptyNodes.insert(info.parent);
		}

		m_emptyNodes.reserve(emptyNodes.size());
		for (auto * meshNode : emptyNodes) {
			auto * parent = meshNode->parent();

			int posIdx = parent->getChildIndex(meshNode);
			assert(posIdx >= 0);

			auto modDelete = sceneModificationDelete(meshNode, posIdx);

			scene.fireBeforeSceneUpdate(modDelete);

			NodePtr removedNode = parent->removeChildNode(static_cast<size_t>(posIdx));
			m_emptyNodes.emplace_back(EmptyNodeInfo{ std::move(removedNode), parent, posIdx });

			scene.fireAfterSceneUpdate(modDelete);
		}
	}

	scene.setSelection(selection, SelectionOperation_Set);

	scene.unlock(SceneInternalModification_Geometries);
}

void MoveMeshesCommand::undo()
{
	auto &scene = m_parentNode->scene();
	scene.lock(SceneInternalModification_Geometries);

	if (m_deleteEmptyNodes) {
		for (auto it = m_emptyNodes.rbegin(); it != m_emptyNodes.rend(); ++it) {
			int insertPos = it->posIdx;
			auto modAdd = sceneModificationAdd(it->node.get(), it->parent, insertPos);

			scene.fireBeforeSceneUpdate(modAdd);

			it->parent->insertChildNode(it->node, it->posIdx);

			scene.fireAfterSceneUpdate(modAdd);
		}

		m_emptyNodes.clear();
	}

	for (auto it = m_geoms.rbegin(); it != m_geoms.rend(); ++it) {
		int modInsertPos = static_cast<int>(it->parent->numChildren(SceneElement_Node)) + it->pos;
		auto modMove = sceneModificationMove(it->geom, it->parent, modInsertPos);

		scene.fireBeforeSceneUpdate(modMove);

		int pos = m_targetNode->getGeometryIndex(it->geom);
		GeometryPtr geomPtr = m_targetNode->removeGeometry(pos);
		it->geom->setVertices(it->vertices);
		it->parent->insertGeometry(geomPtr, it->pos);

		scene.fireAfterSceneUpdate(modMove);
	}

	if (m_targetNode != m_parentNode) {
		int idx = m_parentNode->getChildIndex(m_targetNode);
		assert(idx >= 0);
		auto modDelete = sceneModificationDelete(m_targetNode, idx);

		scene.fireBeforeSceneUpdate(modDelete);

		m_targetNodePtr = m_parentNode->removeChildNode(static_cast<size_t>(idx));

		scene.fireAfterSceneUpdate(modDelete);
	}

	scene.setSelection(m_oldSelection, SelectionOperation_Set);

	scene.unlock(SceneInternalModification_Geometries);
}

// ------------------------------------------------------------------------ //

class SceneStateCommand : public QUndoCommand
{
	Scene &		m_scene;
	QJsonObject	m_oldState, m_newState;
	uint32_t	m_flags;

	void redo() override
	{
		m_scene._setState(m_newState, m_flags);
	}

	void undo() override
	{
		m_scene._setState(m_oldState, m_flags);
	}

public:
	SceneStateCommand(Scene & scene, QJsonObject oldState, QJsonObject newState, uint32_t flags)
		: m_scene(scene)
		, m_oldState(oldState)
		, m_newState(newState)
		, m_flags(flags)
	{
	}
};

void sceneChangeState(Scene &scene, QJsonObject oldState, QJsonObject newState, uint32_t flags)
{
	scene.pushCommand(new SceneStateCommand(scene, oldState, newState, flags));
}

// ------------------------------------------------------------------------ //

class CameraStateCommand : public QUndoCommand
{
	Camera & m_camera;
	CameraState m_oldState, m_newState;

	void redo() override
	{
		m_camera.setState(m_newState);
	}

	void undo() override
	{
		m_camera.setState(m_oldState);
	}

public:
	CameraStateCommand(Camera & camera, const CameraState & oldState, const CameraState & newState)
		: m_camera(camera)
		, m_oldState(oldState)
		, m_newState(newState)
	{
	}
};

void cameraChangeState(Camera &camera, const CameraState & oldState, const CameraState & newState)
{
	camera.pushCommand(new CameraStateCommand(camera, oldState, newState));
}
