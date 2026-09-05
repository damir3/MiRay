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

#include "../ParametersImpl.h"
#include "UVMapping.h"

struct PivotInfo {
	Node *	node;
	mat4	matLocal;
	vec3	pivot;

	struct NodeInfo {
		Node *	node;
		vec3	pos;
	};

	struct GeomInfo {
		Geometry *			geom;
		std::vector<vec3>	vertices;
	};

	std::vector<NodeInfo>	nodes;
	std::vector<GeomInfo>	geometries;

	void restore(bool updateCollisionGeometries);
	void apply();
};

class PivotParameters final : public IPivotParameters, public IParameterOwner
{
	Scene &					m_scene;
	std::vector<PivotInfo>	m_pivots;
	EnumParameterImpl		m_calculatePivot;
	BooleanParameterImpl	m_includingChildrenNodes;
	EnumParameterImpl		m_pivotX;
	EnumParameterImpl		m_pivotY;
	EnumParameterImpl		m_pivotZ;
	bool					m_accept;

	vec3 getPivot(const BBox & bbox) const;
	BBox getOOBB(Node *) const;
	void restore();
	void doIt();

public:
	PivotParameters(Scene & scene, const NodeSelection &meshes);
	~PivotParameters() override;

	EnumParameterImpl &calculatePivot() override { return m_calculatePivot; }
	BooleanParameterImpl &includingChildrenNodes() override { return m_includingChildrenNodes; }
	EnumParameterImpl &pivotX() override { return m_pivotX; }
	EnumParameterImpl &pivotY() override { return m_pivotY; }
	EnumParameterImpl &pivotZ() override { return m_pivotZ; }

	void accept() override;

	CoreInstance & core() const override;
	void pushCommand(QUndoCommand *cmd) override;
	void fireChanged(eParamId paramId) override;
};
