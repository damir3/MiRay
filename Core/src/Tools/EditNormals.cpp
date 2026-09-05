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

#include "EditNormals.h"

static const char * const MAKE_EDGES[] = {
	"Hard", "Soft", "Auto", nullptr};

EditNormals::EditNormals(Scene & scene, const GeomSelection &meshes)
	: m_scene(scene)
	, m_calculateNormals(true, PID_EDIT_NORMALS_CALCULATE_NORMALS, *this)
	, m_makeEdges(2, MAKE_EDGES, PID_EDIT_NORMALS_MAKE_EDGES, *this)
	, m_maxSoftAngle(30.f, 0.f, 180.f, 1, PID_EDIT_NORMALS_MAX_SOFT_ANGLE, *this)
	, m_flipNormals(false, PID_EDIT_NORMALS_FLIP_NORMALS, *this)
	, m_flipFacing(false, PID_EDIT_NORMALS_FLIP_FACING, *this)
	, m_accept(false)
{
	m_maxSoftAngle.setEnabled(true);

	m_geoms.reserve(meshes.size());
	for (auto geom : meshes) {
		GeomInfo gi;
		gi.geom = static_cast<Geometry *>(geom);
		gi.data1.indices = gi.geom->indices();
		gi.data1.vertices = gi.geom->vertices();
		for (int i = 0; i < MAX_UV_SETS; i++)
			gi.data1.uvSets[i] = gi.geom->uvSet(i);

		m_geoms.emplace_back(gi);
		m_meshes.insert(static_cast<MeshNode *>(geom->parent()));
	}

	doIt();
}

EditNormals::~EditNormals()
{
	if (!m_accept)
		restore();
}

// ------------------------------------------------------------------------ //

void EditNormals::restore()
{
	m_scene.lock(SceneInternalModification_Geometries);

	for (auto & gi : m_geoms)
		gi.data1.apply(gi.geom);

	for (auto mesh : m_meshes)
		mesh->updateMeshCollisionGeometries();

	m_scene.unlock(SceneInternalModification_Geometries);
}

struct CompareVec3 {
	bool operator()(const vec3 & a, const vec3 & b) const
	{
		if (a.x < b.x) return true;
		if (a.x > b.x) return false;

		if (a.y < b.y) return true;
		if (a.y > b.y) return false;

		if (a.z < b.z) return true;
		if (a.z > b.z) return false;

		return false;
	}
};

void EditNormals::doIt()
{
	m_scene.lock(SceneInternalModification_Geometries);

	for (auto & gi : m_geoms) {
		auto numIndices = gi.data1.indices.size();
		GeomData & data1 = gi.data1;
		GeomData & data2 = gi.data2;

		if (m_calculateNormals.get()) {
			data2.indices.resize(numIndices);
			data2.vertices.resize(numIndices);
			for (int j = 0; j < MAX_UV_SETS; j++)
				data2.uvSets[j].resize(!data1.uvSets[j].empty() ? numIndices : 0);

			for (size_t i = 0; i < numIndices; i++) {
				auto vi = data1.indices[i];
				data2.indices[i] = (uint32_t)i;
				data2.vertices[i] = data1.vertices[vi];
				for (int j = 0; j < MAX_UV_SETS; j++) {
					if (!data2.uvSets[j].empty())
						data2.uvSets[j][i] = data1.uvSets[j][vi];
				}
			}

			for (size_t i = 0; i < numIndices; i += 3) {
				auto & v0 = data2.vertices[i];
				auto & v1 = data2.vertices[i + 1];
				auto & v2 = data2.vertices[i + 2];
				auto normal = glm::normalize(glm::cross(v2.pos - v0.pos, v1.pos - v0.pos));
				v0.normal = v1.normal = v2.normal = normal;
			}

			if (m_makeEdges.getIndex() > 0 && m_maxSoftAngle.get() > 0.f) {
				const auto minDP = std::cos(glm::radians(m_maxSoftAngle.get()));
				std::map<vec3, std::vector<uint32_t>, CompareVec3> verticesMap;

				for (size_t i = 0; i < numIndices; i++) {
					auto it = verticesMap.find(data2.vertices[i].pos);
					if (it != verticesMap.end())
						it->second.push_back((uint32_t)i);
					else
						verticesMap[data2.vertices[i].pos] = std::vector<uint32_t>{(uint32_t)i};
				}

				for (auto & v : verticesMap) {
					while (v.second.size() > 1) {
						std::vector<uint32_t> indices;
						if (m_makeEdges.getIndex() > 1) {
							indices.reserve(v.second.size());
							indices.push_back(v.second.back());
							v.second.pop_back();
							for (size_t i = 0; i < indices.size(); i++) {
								auto & n = data2.vertices[indices[i]].normal;
								for (auto it = v.second.begin(); it != v.second.end(); ) {
									auto & n2 = data2.vertices[*it].normal;
									if (glm::dot(n, n2) >= minDP) {
										indices.push_back(*it);
										it = v.second.erase(it);
									} else
										++it;
								}
							}
						} else {
							std::swap(indices, v.second);
						}

						auto normal = vec3(0.f);
						for (auto it = indices.begin(); it != indices.end(); ++it)
							normal += data2.vertices[*it].normal;

						normal = glm::normalize(normal);

						for (auto it = indices.begin(); it != indices.end(); ++it)
							data2.vertices[*it].normal = normal;
					}
				}
			}

			data2.optimize();
		} else {
			data2 = data1;
		}

		if (m_flipFacing.get()) {
			for (size_t i = 0; i < numIndices; i += 3)
				std::swap(data2.indices[i + 1], data2.indices[i + 2]);
		}

		if (m_flipNormals.get()) {
			for (auto & v : data2.vertices)
				v.normal = -v.normal;
		}

		data2.apply(gi.geom);
	}

	for (auto mesh : m_meshes)
		mesh->updateMeshCollisionGeometries();

	m_scene.unlock(SceneInternalModification_Geometries);
}

// ------------------------------------------------------------------------ //

void EditNormals::accept()
{
	m_accept = true;
	m_scene.pushCommand(new UpdateGeometriesCommand(m_scene, m_geoms, m_meshes));
}

// ------------------------------------------------------------------------ //

CoreInstance & EditNormals::core() const
{
	return m_scene.core();
}

void EditNormals::pushCommand(QUndoCommand *cmd)
{
	cmd->redo();
	delete cmd;
}

void EditNormals::fireChanged(eParamId paramId)
{
	switch (paramId) {
		case PID_EDIT_NORMALS_CALCULATE_NORMALS:
			m_makeEdges.setEnabled(m_calculateNormals.get());
		case PID_EDIT_NORMALS_MAKE_EDGES:
			m_maxSoftAngle.setEnabled(m_calculateNormals.get() && m_makeEdges.getIndex() == 2);
		case PID_EDIT_NORMALS_MAX_SOFT_ANGLE:
		case PID_EDIT_NORMALS_FLIP_NORMALS:
		case PID_EDIT_NORMALS_FLIP_FACING:
			doIt();
			break;
	}
}
