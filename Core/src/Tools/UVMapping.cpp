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

#include "UVMapping.h"

enum eMapping {
	MAPPING_BOX = 0,
	MAPPING_PLANAR_X,
	MAPPING_PLANAR_Y,
	MAPPING_PLANAR_Z,
	MAPPING_CYLINDRICAL_X,
	MAPPING_CYLINDRICAL_Y,
	MAPPING_CYLINDRICAL_Z,
	MAPPING_SPHERICAL_X,
	MAPPING_SPHERICAL_Y,
	MAPPING_SPHERICAL_Z,
};

enum eFitTo {
	FIT_TO_NODE = 0,
	FIT_TO_GEOMETRY,
	FIT_TO_SCENE,
	FIT_TO_SELECTION,
};

static const char * const MAPPING_TYPES[] = {
	"Box", "Planar X", "Planar Y", "Planar Z",
	"Cylindrical X", "Cylindrical Y", "Cylindrical Z",
	"Spherical X", "Spherical Y", "Spherical Z", nullptr};
static const char * const FIT_TO[] = {"Node", "Geometry", "Scene", "Selection", nullptr};
static const char * const UV_SETS[] = {"UV0", "UV1", "UV2", "UV3", nullptr};

UVMapping::UVMapping(Scene & scene, const GeomSelection &meshes)
	: m_scene(scene)
	, m_mapping(0, MAPPING_TYPES, PID_UV_MAPPING_MAPPING, *this)
	, m_fitTo(0, FIT_TO, PID_UV_MAPPING_MAPPING, *this)
	, m_uvSet(0, UV_SETS, PID_UV_MAPPING_CHANNEL, *this)
	, m_flipU(false, PID_UV_MAPPING_FLIP_U, *this)
	, m_flipV(false, PID_UV_MAPPING_FLIP_V, *this)
	, m_normalize(false, PID_UV_MAPPING_NORMALIZE, *this)
	, m_scale(vec3(1.f), 3, PID_UV_MAPPING_SCALE, *this)
	, m_repeat(vec2(1.f), 3, PID_UV_MAPPING_REPEAT, *this)
	, m_uvsetIndex(0)
	, m_accept(false)
{
	m_geoms.reserve(meshes.size());
	for (auto geom : meshes) {
		GeomInfo gi;
		gi.geom = static_cast<Geometry *>(geom);
		gi.data1.indices = gi.geom->indices();
		gi.data1.vertices = gi.geom->vertices();
		for (int i = 0; i < MAX_UV_SETS; i++)
			gi.data1.uvSets[i] = gi.geom->uvSet(i);

		m_geoms.emplace_back(gi);
		m_meshes.insert(static_cast<MeshNode *>(gi.geom->parent()));
	}

	doIt();
}

UVMapping::~UVMapping()
{
	if (!m_accept)
		restore();
}

// ------------------------------------------------------------------------ //

void UVMapping::restore()
{
	m_scene.lock(SceneInternalModification_Geometries);

	for (auto & gi : m_geoms)
		gi.data1.apply(gi.geom);

	for (auto mesh : m_meshes)
		mesh->updateMeshCollisionGeometries();

	m_scene.unlock(SceneInternalModification_Geometries);
}

void UVMapping::updateBBox(BBox & bbox, const vec3 & nodeScale)
{
	const float MIN_EXT = 1e-6f;
	const auto center = bbox.center();
	auto ext = bbox.size() * 0.5f;
	if (m_normalize.get()) {
		ext *= nodeScale;
		ext = vec3(std::max(std::max(ext.x, ext.y), ext.z));
		ext /= nodeScale;
	}
	ext *= m_scale.get();
	ext.x = std::max(ext.x, MIN_EXT);
	ext.y = std::max(ext.y, MIN_EXT);
	ext.z = std::max(ext.z, MIN_EXT);
	bbox.min = center - ext;
	bbox.max = center + ext;
}

void UVMapping::doIt()
{
	m_uvsetIndex = m_uvSet.getIndex();
	assert(m_uvsetIndex < MAX_UV_SETS);

	m_scene.lock(SceneInternalModification_Geometries);

	auto mapping = m_mapping.getIndex();
	auto fitTo = m_fitTo.getIndex();

	BBox bbox = m_scene.root().oobb();
	if (fitTo == FIT_TO_SCENE) {
		bbox = m_scene.root().oobb();
	} else if (fitTo == FIT_TO_SELECTION) {
		bbox.clear();
		for (auto & gi : m_geoms)
			gi.geom->addToAABB(bbox);
	}

	updateBBox(bbox, vec3(1.f));

	for (auto & gi : m_geoms) {
		auto node = static_cast<Node *>(gi.geom->parent());
		if (fitTo == FIT_TO_NODE) {
			bbox = node->oobb();
			updateBBox(bbox, node->scale().get());
		} else if (fitTo == FIT_TO_GEOMETRY) {
			bbox = gi.geom->bbox();
			updateBBox(bbox, node->scale().get());
		}

		auto numIndices = gi.data1.indices.size();
		GeomData & data2 = gi.data2;
		data2.indices.resize(numIndices);
		data2.vertices.resize(numIndices);
		for (int j = 0; j < MAX_UV_SETS; j++)
			data2.uvSets[j].resize(j == m_uvsetIndex || !gi.data1.uvSets[j].empty() ? numIndices : 0);

		for (size_t i = 0; i < numIndices; i++) {
			auto vi = gi.data1.indices[i];
			data2.indices[i] = (uint32_t)i;
			data2.vertices[i] = gi.data1.vertices[vi];
			for (int j = 0; j < MAX_UV_SETS; j++) {
				if (j != m_uvsetIndex && !data2.uvSets[j].empty())
					data2.uvSets[j][i] = gi.data1.uvSets[j][vi];
			}
		}

		auto invSize = vec3(1.f) / bbox.size();
		auto & uvSet = data2.uvSets[m_uvsetIndex];
		if (mapping == MAPPING_BOX) {
			for (size_t i = 0; i < numIndices; i+=3) {
				const Vertex * v[3] = {
					&gi.data1.vertices[gi.data1.indices[i + 0]],
					&gi.data1.vertices[gi.data1.indices[i + 1]],
					&gi.data1.vertices[gi.data1.indices[i + 2]]
				};

				auto normal = glm::cross(v[2]->pos - v[0]->pos, v[1]->pos - v[0]->pos);
				if (fitTo == FIT_TO_SCENE || fitTo == FIT_TO_SELECTION)
					normal = glm::transformNormal(normal, node->globalTransformation());

				vec3 absNormal(std::fabs(normal.x), std::fabs(normal.y), std::fabs(normal.z));

				for (int j = 0; j < 3; j++) {
					auto & uv = uvSet[i + j];
					auto pos = v[j]->pos;
					if (fitTo == FIT_TO_SCENE || fitTo == FIT_TO_SELECTION)
						pos = glm::transformCoord(pos, node->globalTransformation());

					auto texPos = (pos - bbox.min) * invSize;
					if (absNormal.x > absNormal.y)
						uv = absNormal.x > absNormal.z ? planarMappingX(texPos) : planarMappingZ(texPos);
					else
						uv = absNormal.y > absNormal.z ? planarMappingY(texPos) : planarMappingZ(texPos);

					if (m_flipU.get()) uv.x = 1.f - uv.x;
					if (m_flipV.get()) uv.y = 1.f - uv.y;
				}
			}
		} else {
			for (size_t i = 0; i < numIndices; i++) {
				auto vi = gi.data1.indices[i];
				const auto & v = gi.data1.vertices[vi];
				auto & uv = uvSet[i];

				auto pos = v.pos;
				if (fitTo == FIT_TO_SCENE || fitTo == FIT_TO_SELECTION)
					pos = glm::transformCoord(pos, node->globalTransformation());

				auto texPos = (pos - bbox.min) * invSize;

				switch (mapping) {
//					case MAPPING_BOX:
//					{
//						auto normal = v.normal;
//						if (fitTo == FIT_TO_SCENE || fitTo == FIT_TO_SELECTION)
//							normal = glm::transformNormal(normal, node->GetGlobalTransformation());
//
//						vec3 absNormal(std::fabs(normal.x), std::fabs(normal.y), std::fabs(normal.z));
//						if (absNormal.x > absNormal.y)
//							uv = absNormal.x > absNormal.z ? planarMappingX(texPos) : planarMappingZ(texPos);
//						else
//							uv = absNormal.y > absNormal.z ? planarMappingY(texPos) : planarMappingZ(texPos);
//						break;
//					}

					case MAPPING_PLANAR_X:
						uv = planarMappingX(texPos);
						break;

					case MAPPING_PLANAR_Y:
						uv = planarMappingY(texPos);
						break;

					case MAPPING_PLANAR_Z:
						uv = planarMappingZ(texPos);
						break;

					case MAPPING_CYLINDRICAL_X:
						uv = cylindricalMappingX(texPos);
						break;

					case MAPPING_CYLINDRICAL_Y:
						uv = cylindricalMappingY(texPos);
						break;

					case MAPPING_CYLINDRICAL_Z:
						uv = cylindricalMappingZ(texPos);
						break;

					case MAPPING_SPHERICAL_X:
						uv = sphericalMappingX(texPos);
						break;

					case MAPPING_SPHERICAL_Y:
						uv = sphericalMappingY(texPos);
						break;

					case MAPPING_SPHERICAL_Z:
						uv = sphericalMappingZ(texPos);
						break;
				}

				if (uv.x < 0.f) uv.x += 1.f;
				if (m_flipU.get()) uv.x = 1.f - uv.x;
				if (m_flipV.get()) uv.y = 1.f - uv.y;
			}

			if (mapping >= MAPPING_CYLINDRICAL_X && mapping <= MAPPING_SPHERICAL_Z) {
				for (size_t i = 0; i < numIndices; i += 3) {
					if (std::fabs(uvSet[i + 2].x - uvSet[i + 0].x) > 0.5f ||
						std::fabs(uvSet[i + 2].x - uvSet[i + 1].x) > 0.5f) {
						for (int j = 0; j < 3; j++) {
							auto & v = uvSet[i + j];
							if (v.x > 0.5f)
								v.x -= 1.f;
						}
					}

					for (int j = 0; j < 3; j++) {
						auto & v = uvSet[i + j];
						if (v.y == 0.f || v.y == 1.f) {
							auto & v1 = uvSet[i + (j + 1) % 3];
							auto & v2 = uvSet[i + (j + 2) % 3];
							if (v1.y != v.y && v2.y != v.y)
								v.x = (v1.x + v2.x) * 0.5f;
							break;
						}
					}
				}
			}
		}

		const auto repeat = m_repeat.get();
		for (auto & uv : uvSet)
			uv *= repeat;

		data2.optimize();

		data2.apply(gi.geom);
	}

	for (auto mesh : m_meshes)
		mesh->updateMeshCollisionGeometries();

	m_scene.unlock(SceneInternalModification_Geometries);
}

// ------------------------------------------------------------------------ //

struct CompareVertex {
	static GeomData * data;

	bool operator()(const uint32_t & a, const uint32_t & b) const
	{
		const Vertex & gv1 = data->vertices[a];
		const Vertex & gv2 = data->vertices[b];

		if (gv1.pos.x < gv2.pos.x) return true;
		if (gv1.pos.x > gv2.pos.x) return false;

		if (gv1.pos.y < gv2.pos.y) return true;
		if (gv1.pos.y > gv2.pos.y) return false;

		if (gv1.pos.z < gv2.pos.z) return true;
		if (gv1.pos.z > gv2.pos.z) return false;

		if (gv1.normal.x < gv2.normal.x) return true;
		if (gv1.normal.x > gv2.normal.x) return false;

		if (gv1.normal.y < gv2.normal.y) return true;
		if (gv1.normal.y > gv2.normal.y) return false;

		if (gv1.normal.z < gv2.normal.z) return true;
		if (gv1.normal.z > gv2.normal.z) return false;

		for (int i = 0; i < MAX_UV_SETS; i++) {
			if (data->uvSets[i].empty()) continue;
			const vec2 & uv1 = data->uvSets[i][a];
			const vec2 & uv2 = data->uvSets[i][b];

			if (uv1.x < uv2.x) return true;
			if (uv1.x > uv2.x) return false;

			if (uv1.y < uv2.y) return true;
			if (uv1.y > uv2.y) return false;
		}

		return false;
	}
};

GeomData * CompareVertex::data = nullptr;

void GeomData::copyFrom(Geometry * geom)
{
	vertices = geom->vertices();
	indices = geom->indices();
	for (int i = 0; i < MAX_UV_SETS; i++)
		uvSets[i] = geom->uvSet(i);
}

void GeomData::optimize()
{
	std::vector<uint32_t> newIndices(vertices.size());
	std::map<uint32_t, uint32_t, CompareVertex> verticesMap;
	uint32_t numVertices = 0;

	CompareVertex::data = this;

	for (uint32_t i = 0; i < vertices.size(); i++) {
		auto it = verticesMap.find(i);
		if (it != verticesMap.end()) {
			newIndices[i] = it->second;
		} else {
			newIndices[i] = numVertices;
			verticesMap.insert(std::make_pair(i, numVertices++));
		}
	}

	CompareVertex::data = nullptr;

	assert(numVertices == verticesMap.size());

	LogInformation() << QString("Mesh optimized: %1 -> %2").arg((uint32_t)vertices.size()).arg(numVertices);

	for (size_t i = 0; i < indices.size(); i++) {
		indices[i] = newIndices[indices[i]];
	}

	newIndices.resize(numVertices);
	for (auto it = verticesMap.begin(); it != verticesMap.end(); ++it) {
		newIndices[it->second] = it->first;
	}

	for (uint32_t i = 0; i < numVertices; i++) {
		if (newIndices[i] != i) {
			vertices[i] = vertices[newIndices[i]];
			for (int j = 0; j < MAX_UV_SETS; j++) {
				if (!uvSets[j].empty())
					uvSets[j][i] = uvSets[j][newIndices[i]];
			}
		}
	}

	vertices.resize(numVertices);
	for (int i = 0; i < MAX_UV_SETS; i++) {
		if (!uvSets[i].empty())
			uvSets[i].resize(numVertices);
	}
}

void GeomData::apply(Geometry * geom) const
{
	geom->setIndices(indices);
	geom->setVertices(vertices);
	for (int i = 0; i < MAX_UV_SETS; i++)
		geom->setUVset(i, uvSets[i]);
}

// ------------------------------------------------------------------------ //

void UpdateGeometriesCommand::redo()
{
	if (m_firstTime) {
		m_firstTime = false;
		return;
	}

	m_scene.lock(SceneInternalModification_Geometries);

	for (auto & gi : m_geoms)
		gi.data2.apply(gi.geom);

	for (auto mesh : m_meshes)
		mesh->updateMeshCollisionGeometries();

	m_scene.unlock(SceneInternalModification_Geometries);
}

void UpdateGeometriesCommand::undo()
{
	m_scene.lock(SceneInternalModification_Geometries);

	for (auto & gi : m_geoms)
		gi.data1.apply(gi.geom);

	for (auto mesh : m_meshes)
		mesh->updateMeshCollisionGeometries();

	m_scene.unlock(SceneInternalModification_Geometries);
}

void UVMapping::accept()
{
	m_accept = true;
	m_scene.pushCommand(new UpdateGeometriesCommand(m_scene, m_geoms, m_meshes));
}

// ------------------------------------------------------------------------ //

CoreInstance & UVMapping::core() const
{
	return m_scene.core();
}

void UVMapping::pushCommand(QUndoCommand *cmd)
{
	cmd->redo();
	delete cmd;
}

void UVMapping::fireChanged(eParamId paramId)
{
	switch (paramId) {
		case PID_UV_MAPPING_MAPPING:
		case PID_UV_MAPPING_CHANNEL:
		case PID_UV_MAPPING_FIT_TO:
		case PID_UV_MAPPING_FLIP_U:
		case PID_UV_MAPPING_FLIP_V:
		case PID_UV_MAPPING_NORMALIZE:
		case PID_UV_MAPPING_SCALE:
		case PID_UV_MAPPING_REPEAT:
			doIt();
			break;
	}
}
