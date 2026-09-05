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

#include "AbstractLight.h"
#include "Materials/MaterialImpl.h"

class MeshNode;
class Geometry;

struct GeomData {
	std::vector<uint32_t> indices;
	std::vector<Vertex>   vertices;
	std::vector<vec2>     uvSets[MAX_UV_SETS];

	void copyFrom(Geometry * geom);
	void optimize();
	void apply(Geometry * geom) const;
};

class Geometry final : public IGeometry, public AbstractLight
{
	Q_OBJECT

	QString			m_guid;
	MeshNode *		m_node = nullptr;
	MaterialImpl *	m_material = nullptr;
	BBox			m_bbox;
	bool			m_doubleSided = false;
	float			m_invRealArea = 0.f;
	float			m_invArea = 0.f;
	unsigned int	m_rtcGeomID;
	uint32_t		m_numTriangles = 0;
	std::vector<uint32_t>	m_indices;
	std::vector<Vertex>		m_vertices;
	std::vector<vec2>		m_uvSets[MAX_UV_SETS];
	std::vector<float>		m_lightInfo;

	struct {
		MaterialImpl * material = nullptr;
	} m_animState[2];

	uint32_t getRandomTriangle(float & uSample) const;
	vec3 getEmission(Sampler & sampler, const uint32_t * vi, const vec2 & uv) const;

	static void filterFunc(const struct RTCFilterFunctionNArguments * args);

public:
	Geometry(MeshNode * node, const QString & guid);
	~Geometry() override;

	eSceneElementType type() const override { return SceneElement_Geometry; }
	QString name() const override;

	const QString & guid() const { return m_guid; }
	void _setGuid(const QString & guid) { m_guid = guid; }

	QJsonObject getState(bool fullInfo);
	void setState(const QJsonObject & state);

	void setAnimationState(const QJsonObject & geoms1, const QJsonObject & geoms2, uint32_t flags);
	void setAnimationTime(float time, Sampler & sampler);

	size_t numChildren(eSceneElementType) const override { return 0; };
	ISceneElement * child(size_t, eSceneElementType) const override { return nullptr; }
	INode * parent() const override;
	void setMeshNode(MeshNode *node);
	MeshNode * meshNode() const { return m_node; }

	MaterialImpl * material() const override { return m_material; }
	void setMaterial(IMaterial * material) { m_material = static_cast<MaterialImpl *>(material); }

	void setVertices(const std::vector<Vertex> & vertices) override;
	const decltype(m_vertices) & vertices() const override { return m_vertices; }
	size_t numVertices() const override { return m_vertices.size(); }

	void setIndices(const std::vector<uint32_t> & indices) override;
	const decltype(m_indices) & indices() const override { return m_indices; }
	size_t numIndices() const override { return m_indices.size(); }

	void setUVset(int i, const std::vector<vec2> & uvSet) override;
	const std::vector<vec2> & uvSet(int i) const override { return m_uvSets[i]; }

	size_t numTriangles() const { return m_indices.size() / 3; }

	const BBox & bbox() const { return m_bbox; }
	void addToAABB(BBox & bbox) const;
	void resetArea() { m_invRealArea = 0.f; }

	void createCollisionGeometry();
	void destroyCollisionGeometry();
//	void UpdateTransformation(const mat4 & mat);

	// ------------------------------------------------------------------------ //
	// AbstractLight methods
	// ------------------------------------------------------------------------ //

	vec3 illuminate(const vec3 & aReceivingPosition,
					Sampler & sampler,
					vec3 & oDirectionToLight, float & oDistance,
					float & oDirectPdfW, float * oEmissionPdfW = nullptr,
					float * oCosAtLight = nullptr) const override;

	vec3 illuminateFloor(const vec3 & aReceivingPosition,
					Sampler & sampler,
					vec3 & oDirectionToLight, float & oDistance,
					float & oDirectPdfW, float * oEmissionPdfW = nullptr,
					float * oCosAtLight = nullptr) const override;

	vec3 emitParticle(Sampler & sampler,
					  vec3 & oPosition, vec3 & oDirection,
					  float & oEmissionPdfW, float * oDirectPdfA,
					  float * oCosThetaLight) const override;

	vec3 getRadiance(const Ray & ray,
					 float * oDirectPdfA = nullptr, float * oEmissionPdfW = nullptr,
					 float * oCosThetaLight = nullptr) const override;

	bool isFinite() const override { return true; }

	bool isDelta() const override { return false; }

	unsigned geomID() const override { return m_rtcGeomID; }

	void prepare() override;

	bool isLightSource() const;
};

using GeometryPtr = std::unique_ptr<Geometry>;
using GeometriesList = std::vector<GeometryPtr>;
