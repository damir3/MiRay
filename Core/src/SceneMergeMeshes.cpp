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

#include "Scene.h"
#include "Geometry.h"

class MergeMeshesCommand : public QUndoCommand
{
	MeshNode *	m_targetNode;
	GeometryPtr	m_newGeom;
	Geometry *	m_newGeomRaw;
	Selection	m_oldSelection;
	struct GeomInfo {
		GeometryPtr geom;
		Geometry * geomRaw;
		MeshNode * parent;
		int pos;
	};
	std::vector<GeomInfo> m_oldGeoms;

	void redo() override
	{
		auto &scene = m_targetNode->scene();
		scene.lock(SceneInternalModification_Geometries);

		for (auto & info : m_oldGeoms) {
			info.pos = info.parent->getGeometryIndex(info.geomRaw);
			auto modDelete = sceneModificationDelete(info.geomRaw, info.pos);
			scene.fireBeforeSceneUpdate(modDelete);
			info.geom = info.parent->removeGeometry(static_cast<size_t>(info.pos));
			scene.fireAfterSceneUpdate(modDelete);
		}

		auto pos = static_cast<int>(m_targetNode->numGeometries());
		auto modAdd = sceneModificationAdd(m_newGeomRaw, m_targetNode, pos);
		scene.fireBeforeSceneUpdate(modAdd);
		m_targetNode->insertGeometry(m_newGeom, pos);
		scene.fireAfterSceneUpdate(modAdd);

		scene.setSelection(Selection{m_newGeomRaw}, SelectionOperation_Set);

		scene.unlock(SceneInternalModification_Geometries);
	}

	void undo() override
	{
		auto &scene = m_targetNode->scene();
		scene.lock(SceneInternalModification_Geometries);

		int pos = m_targetNode->getGeometryIndex(m_newGeomRaw);
		auto modDelete = sceneModificationDelete(m_newGeomRaw, pos);
		scene.fireBeforeSceneUpdate(modDelete);
		m_newGeom = m_targetNode->removeGeometry(static_cast<size_t>(pos));
		scene.fireAfterSceneUpdate(modDelete);

		for (auto it = m_oldGeoms.rbegin(); it != m_oldGeoms.rend(); ++it) {
			int modInsertPos = static_cast<int>(it->parent->numChildren(SceneElement_Node)) + it->pos;
			auto modAdd = sceneModificationAdd(it->geomRaw, it->parent, modInsertPos);
			scene.fireBeforeSceneUpdate(modAdd);
			it->parent->insertGeometry(it->geom, it->pos);
			scene.fireAfterSceneUpdate(modAdd);
		}

		scene.setSelection(m_oldSelection, SelectionOperation_Set);

		scene.unlock(SceneInternalModification_Geometries);
	}

public:
	MergeMeshesCommand(const GeomSelection &oldGeoms, Geometry *newGeom, MeshNode * targetNode)
		: m_newGeom(newGeom)
		, m_newGeomRaw(newGeom)
		, m_targetNode(targetNode)
	{
		assert(targetNode);
		m_oldSelection = targetNode->scene().selection();

		m_oldGeoms.reserve(oldGeoms.size());
		for (auto * igeom : oldGeoms) {
			auto * geom = static_cast<Geometry *>(igeom);
			m_oldGeoms.emplace_back(GeomInfo{ nullptr, geom, geom->meshNode(), 0 });
		}
	}
};

IGeometry *Scene::mergeMeshes(const GeomSelection &geoms)
{
	assert(geoms.size() > 1);

	auto *node = static_cast<Geometry *>(geoms.front())->meshNode();
	assert(node);

	size_t numIndices = 0;
	size_t numVertices = 0;
	uint32_t uvMask = 0;
	for (auto *geom : geoms) {
		numIndices += geom->numIndices();
		numVertices += geom->numVertices();
		for (int i = 0; i < MAX_UV_SETS; i++)
			uvMask |= geom->uvSet(i).empty() ? 0 : (1 << i);
	}

	GeomData data;
	data.indices.reserve(numIndices);
	data.vertices.reserve(numVertices);
	for (int i = 0; i < MAX_UV_SETS; i++)
		data.uvSets[i].reserve((uvMask & (1 << i)) ? numVertices : 0);

	for (auto *geom : geoms) {
		const uint32_t vertexOffset = (uint32_t)data.vertices.size();
		for (auto i : geom->indices())
			data.indices.emplace_back(i + vertexOffset);

		const auto  & srcVertices = geom->vertices();
		const auto *geomNode = static_cast<Geometry *>(geom)->meshNode();
		if (geomNode == node) {
			data.vertices.insert(data.vertices.end(), srcVertices.begin(), srcVertices.end());
		} else {
			auto matTransformation = node->inverseGlobalTransformation() * geomNode->globalTransformation();
			auto matTransformationInv = glm::inverse(matTransformation);
			for (auto v : srcVertices) {
				v.pos = glm::transformCoord(v.pos, matTransformation);
				v.normal = glm::normalize(glm::ttransformNormal(v.normal, matTransformationInv));
				data.vertices.emplace_back(v);
			}
		}

		for (int i = 0; i < MAX_UV_SETS; i++) {
			if (uvMask & (1 << i)) {
				auto & dest = data.uvSets[i];
				const auto & src = geom->uvSet(i);
				if (src.empty()) {
					int count = (int)srcVertices.size();
					while (count-- > 0)
						dest.emplace_back(vec2(0.f));
				} else
					dest.insert(dest.end(), src.begin(), src.end());

				assert(dest.size() == data.vertices.size());
			}
		}
	}

	assert(data.indices.size() == numIndices);
	assert(data.vertices.size() == numVertices);

	data.optimize();

	auto *newGeom = new Geometry(node, createGuid());
	newGeom->setMaterial(geoms.front()->material());
	data.apply(newGeom);

	pushCommand(new MergeMeshesCommand(geoms, newGeom, node));
	return newGeom;
}
