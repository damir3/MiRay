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

#include "PivotParameters.h"

static const char * const CALCULATE_PIVOT[] = {
	"Individually for each node", "By all the selected nodes", "By the whole scene", nullptr};

static const char * const PIVOT_POS[] = {"Minimum", "Center", "Maximum", nullptr};

PivotParameters::PivotParameters(Scene & scene, const NodeSelection &nodes)
	: m_scene(scene)
	, m_calculatePivot(0, CALCULATE_PIVOT, PID_PIVOT_PARAMETERS_CALCULATE_PIVOT, *this)
	, m_includingChildrenNodes(false, PID_PIVOT_PARAMETERS_INCLUDING_CHILDREN_NODES, *this)
	, m_pivotX(1, PIVOT_POS, PID_PIVOT_PARAMETERS_PIVOT_X, *this)
	, m_pivotY(1, PIVOT_POS, PID_PIVOT_PARAMETERS_PIVOT_Y, *this)
	, m_pivotZ(1, PIVOT_POS, PID_PIVOT_PARAMETERS_PIVOT_Z, *this)
	, m_accept(false)
{
	m_pivots.reserve(nodes.size());
	for (auto node : nodes) {
		PivotInfo pi;
		pi.node = static_cast<Node *>(node);
		pi.matLocal = node->transformation();
		pi.pivot = glm::translation(pi.matLocal);

		auto numGeoms = node->numChildren(SceneElement_Geometry);
		auto numNodes = node->numChildren(SceneElement_Node);
		pi.nodes.reserve(numNodes);
		pi.geometries.reserve(numGeoms);

		auto numChildren = numGeoms + numNodes;
		for (size_t i = 0; i < numChildren; i++) {
			auto elem = node->child(i, SceneElement_All);
			if (elem->type() == SceneElement_Geometry) {
				PivotInfo::GeomInfo gi;
				gi.geom = static_cast<Geometry *>(elem);
				auto & vertices = gi.geom->vertices();
				gi.vertices.reserve(vertices.size());
				for (auto & v : vertices)
					gi.vertices.push_back(v.pos);

				pi.geometries.emplace_back(gi);
			} else {
				PivotInfo::NodeInfo ni;
				ni.node = static_cast<Node *>(elem);
				ni.pos = glm::translation(ni.node->transformation());
				pi.nodes.emplace_back(ni);
			}
		}

		assert(pi.nodes.size() == numNodes);
		assert(pi.geometries.size() == numGeoms);

		m_pivots.emplace_back(pi);
	}

	doIt();
}

PivotParameters::~PivotParameters()
{
	if (!m_accept)
		restore();
}

// ------------------------------------------------------------------------ //

vec3 PivotParameters::getPivot(const BBox & bbox) const
{
	vec3 pos[] = {bbox.min, bbox.center(), bbox.max};
	return vec3(pos[m_pivotX.getIndex()].x, pos[m_pivotY.getIndex()].y, pos[m_pivotZ.getIndex()].z);
}

BBox PivotParameters::getOOBB(Node * node) const
{
	if (m_includingChildrenNodes.get())
		return node->oobb();

	if (node->isMeshNode()) {
		BBox bbox;
		bbox.clear();
		auto mesh = static_cast<MeshNode *>(node);
		for (const auto & geom : mesh->geometries())
			bbox.addToBounds(geom->bbox());

		if (!bbox.isNull())
			return bbox;
	}

	return BBox(vec3(0.f), vec3(0.f));
}

void PivotParameters::restore()
{
	m_scene.lock(SceneInternalModification_Pivot);

	for (auto & pi : m_pivots)
		pi.restore(true);

	m_scene.unlock(SceneInternalModification_Pivot);
}

void PivotParameters::doIt()
{
	m_scene.lock(SceneInternalModification_Pivot);

	for (auto & pi : m_pivots)
		pi.restore(false);

	vec3 pivot;
	switch (m_calculatePivot.getIndex()) {
		case 1: {
			BBox bbox;
			bbox.clear();
			for (auto & pi : m_pivots) {
				auto aabb = getOOBB(pi.node);
				aabb.transform(pi.node->globalTransformation());
				bbox.addToBounds(aabb);
			}

			pivot = getPivot(bbox);
			break;
		}

		case 2:
			pivot = getPivot(m_scene.root().oobb());
			break;
	}

	for (auto & pi : m_pivots) {
		switch (m_calculatePivot.getIndex()) {
			case 0: {
				auto oobb = getOOBB(pi.node);
				pi.pivot = getPivot(oobb);
				break;
			}

			case 1:
			case 2:
				pi.pivot = glm::transformCoord(pivot, glm::inverse(pi.node->globalTransformation()));
				break;
		}

		pi.apply();
	}

	m_scene.unlock(SceneInternalModification_Pivot);
}

// ------------------------------------------------------------------------ //

void PivotInfo::restore(bool updateCollisionGeometries)
{
	if (node->isMeshNode()) {
		for (auto & gi : geometries) {
			auto vertices = gi.geom->vertices();
			assert(vertices.size() == gi.vertices.size());
			for (size_t i = 0; i < vertices.size(); i++)
				vertices[i].pos = gi.vertices[i];

			gi.geom->setVertices(vertices);
		}

		if (updateCollisionGeometries)
			static_cast<MeshNode *>(node)->updateMeshCollisionGeometries();
	}

	for (auto & ni : nodes) {
		auto mat = ni.node->transformation();
		glm::setTranslation(mat, ni.pos);
		ni.node->setTransformation(mat);
	}

	node->setTransformation(matLocal);
}

void PivotInfo::apply()
{
	if (node->isMeshNode()) {
		for (auto & gi : geometries) {
			auto vertices = gi.geom->vertices();
			for (auto & v : vertices)
				v.pos -= pivot;

			gi.geom->setVertices(vertices);
		}

		static_cast<MeshNode *>(node)->updateMeshCollisionGeometries();
	}

	for (auto & ni : nodes) {
		auto mat = ni.node->transformation();
		glm::setTranslation(mat, glm::translation(mat) - pivot);
		ni.node->setTransformation(mat);
	}

	auto mat = node->transformation();
	glm::setTranslation(mat, glm::transformCoord(pivot, mat));
	node->setTransformation(mat);
}

class UpdatePivotCommand : public QUndoCommand
{
	Scene & m_scene;
	std::vector<PivotInfo>	m_pivots;
	bool	m_firstTime;

	void redo() override
	{
		if (m_firstTime) {
			m_firstTime = false;
			return;
		}

		m_scene.lock(SceneInternalModification_Pivot);

		for (auto & pi : m_pivots)
			pi.restore(false);

		for (auto & pi : m_pivots)
			pi.apply();

		m_scene.unlock(SceneInternalModification_Pivot);
	}

	void undo() override
	{
		m_scene.lock(SceneInternalModification_Pivot);

		for (auto & pi : m_pivots)
			pi.restore(true);

		m_scene.unlock(SceneInternalModification_Pivot);
	}

public:
	UpdatePivotCommand(Scene & scene, std::vector<PivotInfo> & pivots)
		: m_scene(scene)
		, m_pivots(pivots)
		, m_firstTime(true)
	{
	}
};

void PivotParameters::accept()
{
	m_accept = true;
	m_scene.pushCommand(new UpdatePivotCommand(m_scene, m_pivots));
}

// ------------------------------------------------------------------------ //

CoreInstance & PivotParameters::core() const
{
	return m_scene.core();
}

void PivotParameters::pushCommand(QUndoCommand *cmd)
{
	cmd->redo();
	delete cmd;
}

void PivotParameters::fireChanged(eParamId paramId)
{
	switch (paramId) {
		case PID_PIVOT_PARAMETERS_CALCULATE_PIVOT:
		case PID_PIVOT_PARAMETERS_INCLUDING_CHILDREN_NODES:
		case PID_PIVOT_PARAMETERS_PIVOT_X:
		case PID_PIVOT_PARAMETERS_PIVOT_Y:
		case PID_PIVOT_PARAMETERS_PIVOT_Z:
			m_includingChildrenNodes.setEnabled(m_calculatePivot.getIndex() != 2);
			doIt();
			break;
	}
}
