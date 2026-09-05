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

#include "../../Shared/Interfaces/Node.h"
#include "Geometry.h"
#include "ParametersImpl.h"

class Scene;
class IRenderLayer;

using NodePtr = std::unique_ptr<class Node>;
using NodesList = std::vector<NodePtr>;

class Node : public INode, public IParameterOwner
{
	Q_OBJECT
	Q_INTERFACES(INode)

protected:
	Scene &			m_scene;
	QString			m_guid;
	Node			*m_parent;
	NodesList		m_children;
	mat4			m_matLocal;
	mat4			m_matGlobal;
	mat4			m_matInvGlobal;
	BBox			m_oobb; // object oriented bounding box
	bool			m_parentVisible;
	const vec3		m_uniqueColor;
	StringParameterImpl		m_name;
	BooleanParameterImpl	m_visible;
	Vec3ParameterImpl		m_position;
	Vec3ParameterImpl		m_rotation;
	Vec3ParameterImpl		m_scale;

	struct State {
		bool 	visible;
		vec3	position;
		vec3 	rotation;
		vec3	scale;
	} m_animState[2];

	virtual void updateVisibility(bool visible);
	virtual void updateTransformation();
	virtual void calculateBoundingBox();

	virtual QJsonObject getState(bool fullInfo);
	virtual void setState(const QJsonObject & state, uint32_t flags);

	void _setTransformation(const mat4 & matLocal);
	void onTransformationParamChanged();
	void onBoundingBoxChanged();

	virtual void collectBottomVertices(float threshold, vec2 & sumXY, int & count) const;

public:

	Node(const QString & name, Scene & scene, Node * parent);
	~Node();

	eSceneElementType type() const override { return SceneElement_Node; }
	QString name() const final override { return m_name.get(); }
	virtual bool isMeshNode() const { return false; }

	Scene & scene() const { return m_scene; }

	Node * parent() const final override { return m_parent; }
	void setParent(Node * parent);
	bool hasParent(INode * parent) const;

	virtual void getStateRecursive(QJsonObject & nodes, QJsonObject & geoms, bool fullInfo);
	virtual void setStateRecursive(const QJsonObject & nodes, const QJsonObject & geoms, uint32_t flags);

	virtual void setAnimationStateRecursive(const QJsonObject & nodes1, const QJsonObject & nodes2, const QJsonObject & geoms1, const QJsonObject & geoms2, uint32_t flags);
	virtual void setAnimationTimeRecursive(float t, Sampler & sampler);

	const QString & guid() const { return m_guid; }
	void _setGuid(const QString & guid) { m_guid = guid; }

	StringParameterImpl & name() final override { return m_name; }

	BooleanParameterImpl & visible() final override { return m_visible; }
	void _setVisible(bool visible);

	Vec3ParameterImpl & position() final override { return m_position; }
	Vec3ParameterImpl & rotation() final override { return m_rotation; }
	Vec3ParameterImpl & scale() final override { return m_scale; }

	void _setTransformation(const vec3 & position, const vec3 & rotation, const vec3 & scale);

	const mat4 & transformation() const override { return m_matLocal; }
	void setTransformation(const mat4 & matLocal) override;

	const mat4 & globalTransformation() const override { return m_matGlobal; }
	void setGlobalTransformation(const mat4 & matGlobal) override;

	const mat4 & inverseGlobalTransformation() const override { return m_matInvGlobal; }

	const BBox & oobb() const override { return m_oobb; }
	virtual void addToAABB(BBox & bbox, bool checkVisibility) const;
	BBox getAABB(bool checkVisibility = false) const override;

	vec3 getObjectBottomPoint(const BBox & bbox) const override;
	vec3 getObjectBottomPoint() const override;

	const vec3 & uniqueColor() const { return m_uniqueColor; }

	INode *addChildNode(const QString &name, eSceneElementType type) override;

	void insertChildNode(NodePtr & node, int pos);
	NodePtr removeChildNode(size_t pos);
	int getChildIndex(const Node * child) const;

	void pasteNodes(const QByteArray & data) override;

	size_t numChildren(eSceneElementType type) const override;
	ISceneElement * child(size_t i, eSceneElementType type) const override;
	NodesList & children() { return m_children; }
	bool hasChild(INode * node) const;

	void removeChildren();

	virtual void createCollisionGeometries();
	virtual void destroyCollisionGeometries();

	virtual bool isMaterialUsed(const IMaterial * material) const;
	virtual void findGeometriesByMaterial(std::vector<IGeometry *> & geometries, const IMaterial * material) const;

	virtual void findMeshNodesByRenderLayer(std::vector<MeshNode *> & meshNodes, IRenderLayer * renderLayer);
	virtual void findLights(std::vector<AbstractLight *> & lights, bool includeGeomLights);

	void updateBoundingBox();

	// IParameterOwner
	CoreInstance & core() const override;
	void pushCommand(QUndoCommand *cmd) override;
	void fireChanged(eParamId paramId) override;
};
