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

#include "ParametersImpl.h"

class Scene;
class INode;
class Node;
class MaterialImpl;
class Camera;
struct CameraState;
class SnapshotImpl;

class TransformationCommand : public QUndoCommand
{
	Scene &		m_scene;
	bool		m_skipFirstRedo;

	struct AffectedNode {
		INode * node;
		mat4	oldTransform, newTransform;
		mat4	oldGlobalTransform;
		AffectedNode(INode *node, const mat4 & localTransform, const mat4 & globalTransform) : node(node), oldTransform(localTransform), newTransform(localTransform), oldGlobalTransform(globalTransform) {}
	};
	std::vector<AffectedNode>	m_nodes;

	void redo() override;
	void undo() override;

public:
	TransformationCommand(Scene & scene, const NodeSelection & nodes, bool skipFirstRedo = false);

	void setTransformation(const mat4 & mat);
	void setNodeTransformation(INode * targetNode, const mat4 & mat);
};

using NodePtr = std::unique_ptr<class Node>;

class MoveMeshesCommand : public QUndoCommand
{
	const bool m_deleteEmptyNodes;
	Node *     m_parentNode;
	MeshNode * m_targetNode = nullptr;
	NodePtr    m_targetNodePtr;
	Selection  m_oldSelection;
	struct GeomInfo {
		Geometry * geom;
		MeshNode * parent;
		int pos;
		std::vector<Vertex> vertices;

		GeomInfo(Geometry * geom, MeshNode * parent)
			: geom(geom)
			, parent(parent)
			, pos(-1)
			, vertices(geom->vertices())
		{}

		void transformGeomVertices(const mat4 & from, const mat4 & to) const;
	};
	std::vector<GeomInfo> m_geoms;

	struct EmptyNodeInfo {
		NodePtr node;
		Node *  parent;
		int     posIdx;
	};
	std::vector<EmptyNodeInfo> m_emptyNodes;

	void redo() override;
	void undo() override;

public:
	MoveMeshesCommand(const GeomSelection &geoms, bool deleteEmptyNodes, Node * targetNode);

	MeshNode * getTargetNode() const { return m_targetNode; }
};

template<class C, typename T>
class ParamCommand : public QUndoCommand
{
	C & m_owner;
	Scene & m_scene;
	const eParamId m_paramId;
	T & m_value;
	T m_oldValue;
	T m_newValue;

	int id() const override { return m_paramId; }

	void redo() override
	{
		m_scene.lock(SceneInternalModification_Parameter);
		m_value = m_newValue;
		m_owner.fireChanged(m_paramId);
		m_scene.unlock(SceneInternalModification_Parameter);
	}

	void undo() override
	{
		m_scene.lock(SceneInternalModification_Parameter);
		m_value = m_oldValue;
		m_owner.fireChanged(m_paramId);
		m_scene.unlock(SceneInternalModification_Parameter);
	}

	bool mergeWith(const QUndoCommand * other) override
	{
		if (other->id() != id() || &static_cast<const ParamCommand<C, T> *>(other)->m_owner != &m_owner)
			return false;

		m_newValue = static_cast<const ParamCommand<C, T> *>(other)->m_newValue;
		return true;
	}

public:
	ParamCommand(C & owner, Scene & scene, T & value, const T & newValue, eParamId paramId)
		: QUndoCommand(QString("Set parameter %1").arg(paramId))
		, m_owner(owner)
		, m_scene(scene)
		, m_paramId(paramId)
		, m_value(value)
		, m_oldValue(value)
		, m_newValue(newValue)
	{
	}
};

class SceneImportCommand : public QUndoCommand
{
	Scene & m_scene;
	Node &  m_root;
	bool    m_undo;

	std::set<Node *>         m_oldNodes;
	std::set<MaterialImpl *> m_oldMaterials;
	std::set<SnapshotImpl *> m_oldSnapshots;

	std::vector<Node *> m_newNodesRaw;
	NodesList           m_newNodes;
	MaterialsList       m_newMaterials;
	SnapshotsList       m_newSnapshots;

	void toggleSceneState();

	void redo() override;
	void undo() override;

public:
	SceneImportCommand(Scene & scene, Node & root);
};

void sceneCreateNode(Scene & s, NodePtr & node);
void sceneDeleteElements(Scene &s, const Selection &elements);
void sceneMoveNodes(Scene &scene, Node *target, size_t pos, const QString &groupName, const NodeSelection &nodes);
void sceneShowNodes(Scene &s, const NodeSelection &nodes, bool show);
void sceneSetMaterial(Scene &s, const GeomSelection & geometries, IMaterial * material);
void sceneChangeState(Scene &scene, QJsonObject oldState, QJsonObject newState, uint32_t flags);
void cameraChangeState(Camera &camera, const CameraState & oldState, const CameraState & newState);
