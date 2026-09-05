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

#include "Geometry.h"
#include "Materials/MaterialManager.h"

Geometry::Geometry(MeshNode * node, const QString & guid)
	: m_node(node)
	, m_guid(guid)
	, m_rtcGeomID(RTC_INVALID_GEOMETRY_ID)
{
	m_bbox.clear();
	assert(m_node);
}

Geometry::~Geometry()
{
	assert(m_rtcGeomID == RTC_INVALID_GEOMETRY_ID);
}

// ------------------------------------------------------------------------ //

QString Geometry::name() const
{
	return QString("mesh %1").arg(m_node->getGeometryIndex(this));
}

INode * Geometry::parent() const
{
	return m_node;
}

// ------------------------------------------------------------------------ //

QJsonObject Geometry::getState(bool fullInfo)
{
	QJsonObject state;
	state["m"] = m_material->name().get();
	return state;
}

void Geometry::setState(const QJsonObject & state)
{
	auto material = m_node->scene().materialManager().getByName(state["m"].toString());
	if (material)
		m_material = material;
}

void Geometry::setAnimationState(const QJsonObject & geoms1, const QJsonObject & geoms2, uint32_t flags)
{
	if (flags & SF_HAS_ASSIGNED_MATERIALS) {
		auto state1 = geoms1[m_guid].toObject();
		auto state2 = geoms2[m_guid].toObject();
		auto & materialManager = m_node->scene().materialManager();
		auto * material1 = materialManager.getByName(state1["m"].toString());
		auto * material2 = materialManager.getByName(state2["m"].toString());
		m_animState[0].material = material1 ? material1 : m_material;
		m_animState[1].material = material2 ? material2 : m_material;
	} else {
		m_animState[0].material = m_animState[1].material = m_material;
	}
}

void Geometry::setAnimationTime(float time, Sampler & sampler)
{
	m_material = m_animState[sampler.generate1D() > time ? 0 : 1].material;
}

// ------------------------------------------------------------------------ //

void Geometry::setMeshNode(MeshNode *node)
{
	assert(m_rtcGeomID == RTC_INVALID_GEOMETRY_ID);
	m_node = node;
}

void Geometry::setVertices(const std::vector<Vertex> & vertices)
{
	m_vertices = vertices;
	m_invRealArea = 0.f;
	m_bbox.clear();
	for (auto & v : m_vertices) {
		v.normal = glm::normalize(v.normal);
		m_bbox.addToBounds(v.pos);
	}

	m_bbox.min -= 1e-6f;
	m_bbox.max += 1e-6f;
}

void Geometry::setIndices(const std::vector<uint32_t> & indices)
{
	m_invRealArea = 0.f;
	m_indices = indices;
}

void Geometry::setUVset(int i, const std::vector<vec2> & uvSet)
{
	assert(i >= 0 && i < MAX_UV_SETS);
	assert(uvSet.size() == m_vertices.size() || uvSet.empty());
	m_uvSets[i] = uvSet;
	for (auto & tc : m_uvSets[i]) {
		if (!isfinite(tc.x))	tc.x = 0.f;
		if (!isfinite(tc.y))	tc.y = 0.f;
	}
}

// ------------------------------------------------------------------------ //

void Geometry::addToAABB(BBox & bbox) const
{
	auto & mat = m_node->globalTransformation();
	for (auto & v : m_vertices) {
		auto p = glm::transformCoord(v.pos, mat);
		bbox.addToBounds(p);
	}
}

// ------------------------------------------------------------------------ //

void Geometry::createCollisionGeometry()
{
	if (m_rtcGeomID == RTC_INVALID_GEOMETRY_ID) {
		auto rtcGeom = rtcNewGeometry(m_node->rtcDevice(), RTC_GEOMETRY_TYPE_TRIANGLE);
		rtcSetGeometryUserData(rtcGeom, this);
		rtcSetGeometryBuildQuality(rtcGeom, RTC_BUILD_QUALITY_HIGH);

		rtcSetGeometryIntersectFilterFunction(rtcGeom, (RTCFilterFunctionN)&Geometry::filterFunc);
		rtcSetSharedGeometryBuffer(rtcGeom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, m_vertices.data(), 0, sizeof(Vertex), m_vertices.size());
		rtcSetSharedGeometryBuffer(rtcGeom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, m_indices.data(), 0, 3 * sizeof(uint32_t), m_indices.size() / 3);
		rtcCommitGeometry(rtcGeom);

		m_rtcGeomID = rtcAttachGeometry(m_node->rtcScene(), rtcGeom);
		assert(m_rtcGeomID != RTC_INVALID_GEOMETRY_ID);
		rtcReleaseGeometry(rtcGeom);
	}
}

void Geometry::destroyCollisionGeometry()
{
	if (m_rtcGeomID != RTC_INVALID_GEOMETRY_ID) {
		assert(m_node && m_node->rtcScene());
		rtcDetachGeometry(m_node->rtcScene(), m_rtcGeomID);
		m_rtcGeomID = RTC_INVALID_GEOMETRY_ID;
	}
}

// ------------------------------------------------------------------------ //
// AbstractLight implementation
// ------------------------------------------------------------------------ //

uint32_t Geometry::getRandomTriangle(float & uSample) const
{
	if (m_lightInfo.empty()) {
		uSample *= (float)m_numTriangles;
		uint32_t i = std::min((uint32_t)uSample, m_numTriangles - 1);
		uSample -= (float)i;
		assert(uSample >= 0.f && uSample <= 1.f);
		return i;
	}

	uint32_t i1 = 0;
	uint32_t i2 = m_numTriangles;
	while (i2 - i1 > 1) {
		auto im = (i1 + i2) >> 1;
		if (uSample < m_lightInfo[im])
			i2 = im;
		else
			i1 = im;
	}

	uSample = (uSample - m_lightInfo[i1]) / ((i2 < m_numTriangles ? m_lightInfo[i2] : 1.f) - m_lightInfo[i1]);
	assert(uSample >= 0.f && uSample <= 1.f);

	return i1;
}

vec3 Geometry::illuminate(const vec3 & aReceivingPosition,
						  Sampler & sampler,
						  vec3 & oDirectionToLight, float & oDistance,
						  float & oDirectPdfW, float * oEmissionPdfW,
						  float * oCosAtLight) const
{
	auto uSample = sampler.generate2D();
	const auto ti = getRandomTriangle(uSample.x) * 3;
	bool flipNormal = false;
	if (m_doubleSided) {
		if (uSample.y < 0.5f) {
			flipNormal = true;
			uSample.y *= 2.f;
		} else {
			uSample.y = uSample.y * 2.f - 1.f;
		}
	}
	const auto uv = uniformSampleTriangle(uSample.x, uSample.y);
	auto intensity = getEmission(sampler, m_indices.data() + ti, uv);
	if (isBlack(intensity))
		return vec3(0.f);

	const auto & mat = m_node->globalTransformation();
	auto p0 = glm::transformCoord(m_vertices[m_indices[ti + 0]].pos, mat);
	auto p1 = glm::transformCoord(m_vertices[m_indices[ti + 1]].pos, mat);
	auto p2 = glm::transformCoord(m_vertices[m_indices[ti + 2]].pos, mat);

	const auto e1 = p1 - p0;
	const auto e2 = p2 - p0;
	auto normal = glm::cross(e2, e1);
	if (flipNormal) normal *= -1.f;
	auto area = length(normal);
	normal *= 1.f / area;
	area *= 0.5f;

	const auto lightPoint = p0 + e1 * uv.x + e2 * uv.y + normal * 1e-3f;
	oDirectionToLight	= lightPoint - aReceivingPosition;
	const auto distSqr	= glm::length2(oDirectionToLight);
	oDistance			= std::sqrt(distSqr);
	oDirectionToLight	= oDirectionToLight / oDistance;

	const auto cosNormalDir = glm::dot(normal, -oDirectionToLight);

	// too close to, or under, tangent
	if (cosNormalDir < EPS_COSINE)
		return vec3(0.f);

	oDirectPdfW = m_invArea * distSqr / cosNormalDir;

	if (oCosAtLight)
		*oCosAtLight = cosNormalDir;

	if (oEmissionPdfW)
		*oEmissionPdfW = m_invArea * cosNormalDir * M_1_PIf;

	if (m_lightInfo.empty())
		intensity *= area * (float)m_numTriangles * m_invArea; // probability correction

	return intensity;
}

vec3 Geometry::illuminateFloor(const vec3 & aReceivingPosition,
							   Sampler & sampler,
							   vec3 & oDirectionToLight, float & oDistance,
							   float & oDirectPdfW, float * oEmissionPdfW,
							   float * oCosAtLight) const
{
	return illuminate(aReceivingPosition, sampler, oDirectionToLight, oDistance, oDirectPdfW, oEmissionPdfW, oCosAtLight);
}

vec3 Geometry::emitParticle(Sampler & sampler,
							vec3 & oPosition, vec3 & oDirection,
							float & oEmissionPdfW, float * oDirectPdfA,
							float * oCosThetaLight) const
{
	auto rndPosSamples = sampler.generate2D();
	auto rndDirSamples = sampler.generate2D();

	const auto ti = getRandomTriangle(rndPosSamples.x) * 3;
	bool flipNormal = false;
	if (m_doubleSided) {
		if (rndPosSamples.y < 0.5f) {
			flipNormal = true;
			rndPosSamples.y *= 2.f;
		} else {
			rndPosSamples.y = rndPosSamples.y * 2.f - 1.f;
		}
	}
	const auto uv = uniformSampleTriangle(rndPosSamples.x, rndPosSamples.y);
	auto intensity = getEmission(sampler, m_indices.data() + ti, uv);
	if (isBlack(intensity))
		return vec3(0.f);

	const auto & mat = m_node->globalTransformation();
	auto p0 = glm::transformCoord(m_vertices[m_indices[ti + 0]].pos, mat);
	auto p1 = glm::transformCoord(m_vertices[m_indices[ti + 1]].pos, mat);
	auto p2 = glm::transformCoord(m_vertices[m_indices[ti + 2]].pos, mat);

	const auto e1 = p1 - p0;
	const auto e2 = p2 - p0;
	auto normal = glm::cross(e2, e1);
	if (flipNormal) normal *= -1.f;
	auto area = length(normal);
	normal *= 1.f / area;
	area *= 0.5f;

	oPosition = p0 + e1 * uv.x + e2 * uv.y + normal * 1e-3f;

	auto localDirOut = cosineSampleHemisphere(rndDirSamples, &oEmissionPdfW);
	if (oEmissionPdfW == 0.f)
		return vec3(0.f);

	oEmissionPdfW *= m_invArea;

	// cannot really not emit the particle, so just bias it to the correct angle
	localDirOut.z = std::max(localDirOut.z, EPS_COSINE);

	auto dirX = glm::perpendicular(normal);
	auto dirY = glm::cross(dirX, normal);
	oDirection = dirX * localDirOut.x + dirY * localDirOut.y + normal * localDirOut.z;

	if (oDirectPdfA)
		*oDirectPdfA = m_invArea;

	if (oCosThetaLight)
		*oCosThetaLight = localDirOut.z;

	if (m_lightInfo.empty())
		intensity *= area * (float)m_numTriangles * m_invArea; // probability correction

	return intensity * localDirOut.z;
}

vec3 Geometry::getRadiance(const Ray & ray,
						   float * oDirectPdfA, float * oEmissionPdfW,
						   float * oCosThetaLight) const
{
	if (oDirectPdfA)
		*oDirectPdfA = m_invArea;

	if (oEmissionPdfW) {
		*oEmissionPdfW = cosineHemispherePdfW(ray.geomNormal, -ray.direction());
		*oEmissionPdfW *= m_invArea;
	}

	assert(oCosThetaLight == nullptr);

	return vec3(0.f);
}

void Geometry::prepare()
{
	if (m_invRealArea == 0.f) {
		auto & mat = m_node->globalTransformation();
		m_numTriangles = (uint32_t)numTriangles();
		if (m_numTriangles <= 0x10000)
			m_lightInfo.resize(m_numTriangles);

		auto totalArea = 0.f;
		for (size_t ti = 0; ti < m_numTriangles; ti++) {
			if (!m_lightInfo.empty())
				m_lightInfo[ti] = totalArea;

			auto i = ti * 3;
			auto p0 = glm::transformCoord(m_vertices[m_indices[i + 0]].pos, mat);
			auto p1 = glm::transformCoord(m_vertices[m_indices[i + 1]].pos, mat);
			auto p2 = glm::transformCoord(m_vertices[m_indices[i + 2]].pos, mat);
			totalArea += glm::length(glm::cross(p2 - p0, p1 - p0)) * 0.5f;
		}

		m_invRealArea = 1.f / totalArea;

		for (auto & f : m_lightInfo)
			f *= m_invRealArea;
	}

	m_doubleSided = m_material->doubleSided().get() && (m_material->isEmpty() || m_material->isOpaque());
	m_invArea = m_doubleSided ? m_invRealArea * 0.5f : m_invRealArea;
}

bool Geometry::isLightSource() const
{
	for (const auto & group : m_material->groups()) {
		if (group->enabled().get() && group->isEmissive())
			return true;
	}

	return false;
}

vec3 Geometry::getEmission(Sampler & sampler, const uint32_t * vi, const vec2 & uv) const
{
	Ray ray;
	ray.vi[0] = vi[0];
	ray.vi[1] = vi[1];
	ray.vi[2] = vi[2];
	ray.hit.u = uv.x;
	ray.hit.v = uv.y;
	ray.geom = this;
	vec3 emission(0.f);
	float groupTransmission = 1.f;

	for (const auto & group : m_material->groups()) {
		if (group->enabled().get()) {
			auto groupMask = group->mask().getScalar(ray);
			if (groupMask > 0.f && group->isEmissive()) {
				float transmission = groupTransmission * groupMask;
				for (auto layer : group->layers()) {
					if (layer->specularLayer().get())
						break; // ignore emission under the specular layer

					float layerMask = layer->mask().getScalar(ray);
					if (layer->diffuseLayer().get())
						layerMask *= layer->diffuseColor().getOpacity(ray);

					if (layer->isEmissive())
						emission += layer->emissiveColor().getColor(ray) * (transmission * (layer->emissiveIntensity().value() * layerMask));

					if (layer->diffuseLayer().get()) {
						transmission *= 1.f - layerMask;
						if (transmission < EPS_COLOR)
							break;
					}
				}
			}
			groupTransmission *= 1.f - groupMask;
		}
	}

	return emission;
}

void Geometry::filterFunc(const RTCFilterFunctionNArguments * args)
{
	assert(args->N == 1);
	auto geom = static_cast<Geometry *>(args->geometryUserPtr);
	auto ray = reinterpret_cast<Ray *>(args->ray);
	auto hit = reinterpret_cast<RTCHit *>(args->hit);
	*(vec3 *)&hit->Ng_x *= -1.f; // flip geometry normal
	bool hitBack = glm::dot(ray->direction(), *(vec3 *)&hit->Ng_x) > 0;

	if (hitBack && !geom->m_material->doubleSided().get() && !ray->normalMap && !ray->hitBoth) {
		auto boundaryStack = static_cast<BoundaryStack *>(ray->boundaryStack);
		if (!boundaryStack || !boundaryStack->Contains(StackElement(geom->m_material), geom->m_material->priority().get())) {
			args->valid[0] = 0;
			return;
		}
	}

	ray->hitBack = hitBack;
	ray->geom = geom;
}
