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

#include "LightNode.h"

LightNode::LightNode(const QString & name, Scene & scene, Node * parent)
	: Node(name, scene, parent)
	, m_color(vec3(1.f), PID_LIGHT_COLOR, *this)
	, m_intensity(1.f, 0.f, FLT_MAX, 2, PID_LIGHT_INTENSITY, *this)
	, m_radius(1.f, 0.f, FLT_MAX, 2, PID_LIGHT_RADIUS, *this)
	, m_specularIntensity(0.f)
	, m_geomRadius(1.f)
	, m_invArea(0.f)
	, m_rtcGeometry(nullptr)
	, m_geomID(RTC_INVALID_GEOMETRY_ID)
{
}

LightNode::~LightNode()
{
	if (m_geomID != RTC_INVALID_GEOMETRY_ID)
		rtcDetachGeometry(m_scene.rtcScene(), m_geomID);

	if (m_rtcGeometry)
		rtcReleaseGeometry(m_rtcGeometry);
}

// ------------------------------------------------------------------------ //

bool LightNode::senerateSample(const vec3 & target, float radius, const vec2 & ur, vec3 & pos, vec3 & normal) const
{
	const auto & origin = glm::translation(m_matGlobal);
	auto dirZ = target - origin;
	auto dist = length(dirZ);
	dirZ *= 1.f / dist; // normalize
	assert(dist > 0.f && !isnan(dist));
	if (dist <= radius)
		return false;

	auto uz = lerp(radius / dist, 1.f, ur.x);
	auto v = uniformSampleHemisphere(uz, ur.y);
	auto dirX = glm::perpendicular(dirZ);
	auto dirY = glm::cross(dirX, dirZ);
	auto dir = dirX * v.x + dirY * v.y + dirZ * v.z;
	pos = origin + dir * (radius + 1e-3f);
	normal = dir;
	return true;
}

vec3 LightNode::illuminate(const vec3 & aReceivingPosition,
						   Sampler & sampler,
						   vec3 & oDirectionToLight, float & oDistance,
						   float & oDirectPdfW, float * oEmissionPdfW,
						   float * oCosAtLight) const
{
	const float MIN_RADIUS_DIST_RATIO2 = 1e-5f;
	const auto & origin = glm::translation(m_matGlobal);
	auto radius = getGeomRadius();
	auto radius2 = sqr(radius);
	float dist2 = glm::length2(origin - aReceivingPosition);
	auto ratio2 = radius2 / dist2;
	if (ratio2 < MIN_RADIUS_DIST_RATIO2) {
		ratio2 = MIN_RADIUS_DIST_RATIO2;
		radius2 = ratio2 * dist2;
		radius = std::sqrt(radius2);
	}

//	auto dirZ = aReceivingPosition - origin;
//	auto dist = dirZ.Normalize();
//	auto uz = lerp(radius / dist, 1.f, aRndTuple.x);
//	auto v = uniformSampleHemisphere(uz, aRndTuple.y);
//	auto dirX = dirZ; dirX.Perpendicular();
//	auto dirY = glm::cross(dirX, dirZ);
//	auto normal = dirX * v.x + dirY * v.y + dirZ * v.z;
//	auto lightPoint = origin + normal * (radius + 1e-3f);
//	auto h = (1.f - (radius / dist)) * radius;
//	auto invArea = m_invArea;// 1.f / (M_2PIf * radius * h);

	vec3 lightPos;
	vec3 normal;
	const auto aRndTuple = sampler.generate2D();
	if (!senerateSample(aReceivingPosition, radius + 1e-2f, aRndTuple, lightPos, normal))
		return vec3(0.f);

	float sinThetaMax2 = ratio2;
	float cosThetaMax = std::sqrt(std::max(0.f, 1.f - sinThetaMax2));
	if (cosThetaMax >= 1.f)
		return vec3(0.f);

	oDirectPdfW = uniformConePdf(cosThetaMax);
	if (!isfinite(oDirectPdfW))
		return vec3(0.f);

	oDirectionToLight = lightPos - aReceivingPosition;
	auto distSqr = glm::length2(oDirectionToLight);
	oDistance = std::sqrt(distSqr);
	oDirectionToLight = oDirectionToLight / oDistance;
	assert(isfinite(oDirectionToLight.x));

	if (oCosAtLight)
		*oCosAtLight = 1.f;

	if (oEmissionPdfW)
		*oEmissionPdfW = 1.f / (4.f * M_PIf * radius2);

	auto specularIntensity = m_color.rgb() * (m_intensity.value() / radius2);
	//auto i1 = specularIntensity.x / (M_PIf * oDirectPdfW);
	//auto i2 = m_intensity.value() / glm::length2(aReceivingPosition - origin);
	return specularIntensity;
}

vec3 LightNode::illuminateFloor(const vec3 & aReceivingPosition,
								Sampler & sampler,
								vec3 & oDirectionToLight, float & oDistance,
								float & oDirectPdfW, float * oEmissionPdfW,
								float * oCosAtLight) const
{
	return illuminate(aReceivingPosition, sampler, oDirectionToLight, oDistance, oDirectPdfW, oEmissionPdfW, oCosAtLight);
}

vec3 LightNode::emitParticle(Sampler & sampler,
							 vec3 & oPosition, vec3 & oDirection,
							 float & oEmissionPdfW, float * oDirectPdfA,
							 float * oCosThetaLight) const
{
	const auto rndDirSamples = sampler.generate2D();
	const auto rndPosSamples = sampler.generate2D();

	const auto & origin = glm::translation(m_matGlobal);
	auto dirZ = uniformSampleSphere(rndPosSamples.x, rndPosSamples.y);
	auto dirX = glm::perpendicular(dirZ);
	auto dirY = glm::cross(dirX, dirZ);

	auto radius = getGeomRadius();
	oPosition = origin + dirZ * (radius + 1e-2f);

	vec3 localDirOut = cosineSampleHemisphere(rndDirSamples, nullptr);

	// cannot really not emit the particle, so just bias it to the correct angle
	localDirOut.z = std::max(localDirOut.z, EPS_COSINE);

	oDirection = dirX * localDirOut.x + dirY * localDirOut.y + dirZ * localDirOut.z;

	oEmissionPdfW = m_invArea * M_1_PIf;

	if (oDirectPdfA)
		*oDirectPdfA = m_invArea;

	if (oCosThetaLight)
		*oCosThetaLight = 1.f;

	return m_specularIntensity;
}

vec3 LightNode::getRadiance(const Ray & ray,
							float * oDirectPdfA, float * oEmissionPdfW,
							float * oCosThetaLight) const
{
	if (oDirectPdfA)
		*oDirectPdfA = m_invArea;

	const auto & origin = glm::translation(m_matGlobal);

	if (oEmissionPdfW) {
		auto radius = getGeomRadius();
		double sinThetaMax2 = (double)sqr(radius) / (double)glm::length(origin - ray.hitPoint);
		double cosThetaMax = std::sqrt(std::max(0.0, 1.0 - sinThetaMax2));
		*oEmissionPdfW = uniformConePdf(cosThetaMax);
	}

	if (oCosThetaLight)
		*oCosThetaLight = 1.f;

	return m_specularIntensity;
}

// ------------------------------------------------------------------------ //

void lightBoundsFunc(const RTCBoundsFunctionArguments * args)
{
	LightNode* lightNode = static_cast<LightNode *>(args->geometryUserPtr);
	auto * bounds = args->bounds_o;
	const auto & origin = glm::translation(lightNode->globalTransformation());
	auto radius = lightNode->getGeomRadius();
	bounds->lower_x = origin.x - radius;
	bounds->lower_y = origin.y - radius;
	bounds->lower_z = origin.z - radius;
	bounds->upper_x = origin.x + radius;
	bounds->upper_y = origin.y + radius;
	bounds->upper_z = origin.z + radius;
}

void lightIntersectFunc(const RTCIntersectFunctionNArguments * args)
{
	assert(args->N == 1);
	if (!args->valid[0])
		return;

	Ray & ray = *(Ray *)args->rayhit;
	if ((ray.ray.mask & RAY_MASK_LIGHT) == 0)
		return;

	const auto * lightNode = static_cast<const LightNode *>(args->geometryUserPtr);
	const auto radius = lightNode->getGeomRadius();
	assert(radius > 0.f);
	const auto & pos = glm::translation(lightNode->globalTransformation());
	auto v = ray.origin() - pos;
	auto C = glm::length2(v) - sqr(radius);
	if (C < 0.f) return; // ray inside sphere
	auto B = 2.f * glm::dot(v, ray.direction());
	auto D = B * B - 4.f * C;
	if (D < 0.f) return; // reports miss
	auto Q = std::sqrt(D);

	auto t0 = (-B - Q) * 0.5f;
	if ((ray.ray.tnear < t0) & (t0 < ray.ray.tfar)) { // front side
		ray.hit.geomID = lightNode->geomID();
		ray.hit.primID = RTC_INVALID_GEOMETRY_ID;
		ray.ray.tfar = t0;
		*(vec3 *)&ray.hit.Ng_x = glm::normalize(v + ray.direction() * t0);
		return;
	}

//	auto t1 = (-B + Q) * 0.5f;
//	if ((ray.tnear < t1) & (t1 < ray.tfar))
//	{// back side
//		ray.hit.geomID = lightNode->geomID(); // report hit
//		ray.hit.primID = RTC_INVALID_GEOMETRY_ID;
//		ray.tfar = t1;
//		*(vec3 *)ray.Ng = vec3::Normalize(v + *(vec3 *)ray.dir * t1);
//		return;
//	}
}

void lightOccludedFunc(const RTCOccludedFunctionNArguments * args)
{
	assert(args->N == 1);
	if (!args->valid[0])
		return;

	Ray & ray = *(Ray *)args->ray;
	if ((ray.ray.mask & RAY_MASK_LIGHT) == 0)
		return;

	const auto * lightNode = static_cast<const LightNode *>(args->geometryUserPtr);
	const auto & pos = glm::translation(lightNode->globalTransformation());
	const auto radius = lightNode->getGeomRadius();
	assert(radius > 0.f);
	auto v = ray.origin() - pos;
	auto C = glm::length2(v) - sqr(radius);
	if (C < 0.f) return; // ray inside sphere
	auto B = 2.f * glm::dot(v, ray.direction());
	auto D = B * B - 4.f * C;
	if (D < 0.f) return; // reports miss
	auto Q = std::sqrt(D);

	auto t0 = (-B - Q) * 0.5f;
	if ((ray.ray.tnear < t0) & (t0 < ray.ray.tfar)) { // front side
//		args->geomID = lightNode->geomID(); // report hit
//		args->primID = RTC_INVALID_GEOMETRY_ID;
		ray.hit.geomID = lightNode->geomID(); // report hit
		ray.hit.primID = RTC_INVALID_GEOMETRY_ID;
		return;
	}

//	auto t1 = (-B + Q) * 0.5f;
//	if ((ray.tnear < t1) & (t1 < ray.tfar))
//	{// back side
//		ray.hit.geomID = lightNode->geomID(); // report hit
//		ray.hit.primID = RTC_INVALID_GEOMETRY_ID;
//		return;
//	}
}

// ------------------------------------------------------------------------ //

void LightNode::createCollisionGeometries()
{
	if (!m_visible)
		return;

	Node::createCollisionGeometries();

	if (m_geomID == RTC_INVALID_GEOMETRY_ID) {
		m_rtcGeometry = rtcNewGeometry(m_scene.rtcDevice(), RTC_GEOMETRY_TYPE_USER);
		rtcSetGeometryUserPrimitiveCount(m_rtcGeometry, 1);
		rtcSetGeometryUserData(m_rtcGeometry, this);

		rtcSetGeometryBoundsFunction(m_rtcGeometry, &lightBoundsFunc, this);
		rtcSetGeometryIntersectFunction(m_rtcGeometry, &lightIntersectFunc);
		rtcSetGeometryOccludedFunction(m_rtcGeometry, &lightOccludedFunc);
		rtcCommitGeometry(m_rtcGeometry);

		m_geomID = rtcAttachGeometry(m_scene.rtcScene(), m_rtcGeometry);
		rtcCommitScene(m_scene.rtcScene());
	}
}

void LightNode::destroyCollisionGeometries()
{
	Node::destroyCollisionGeometries();

	if (m_geomID != RTC_INVALID_GEOMETRY_ID) {
		rtcDetachGeometry(m_scene.rtcScene(), m_geomID);
		m_geomID = RTC_INVALID_GEOMETRY_ID;
	}

	if (m_rtcGeometry) {
		rtcReleaseGeometry(m_rtcGeometry);
		m_rtcGeometry = nullptr;
	}
}

void LightNode::updateVisibility(bool parentVisible)
{
	Node::updateVisibility(parentVisible);

	if (parentVisible & m_visible.get()) {
		if (m_geomID == RTC_INVALID_GEOMETRY_ID && m_rtcGeometry)
			m_geomID = rtcAttachGeometry(m_scene.rtcScene(), m_rtcGeometry);
		else
			createCollisionGeometries();
	} else {
		if (m_geomID != RTC_INVALID_GEOMETRY_ID) {
			rtcDetachGeometry(m_scene.rtcScene(), m_geomID);
			m_geomID = RTC_INVALID_GEOMETRY_ID;
		}
	}
}

void LightNode::calculateBoundingBox()
{
	Node::calculateBoundingBox();

	m_oobb.addToBounds(vec3(-m_radius.get()));
	m_oobb.addToBounds(vec3(m_radius.get()));
	m_geomRadius = std::max(m_radius.get(), 0.1f);
}

void LightNode::updateTransformation()
{
	Node::updateTransformation();

	if (m_rtcGeometry)
		rtcCommitGeometry(m_rtcGeometry);
}

void LightNode::prepare()
{
	m_specularIntensity = m_color.rgb() * (m_intensity.value() / sqr(m_geomRadius));
	m_invArea = 1.f / (4.f * M_PIf * sqr(m_geomRadius));
}

void LightNode::findLights(std::vector<AbstractLight *> & lights, bool includeGeomLights)
{
	if (!isBlack(m_color.rgb() * m_intensity.value()))
		lights.push_back(this);

	Node::findLights(lights, includeGeomLights);
}

// ------------------------------------------------------------------------ //

void LightNode::fireChanged(eParamId paramId)
{
	switch (paramId) {
		case PID_LIGHT_COLOR:
		case PID_LIGHT_INTENSITY:
			break;
		case PID_LIGHT_RADIUS:
			m_scene.lock(SceneInternalModification_Transformation);

			calculateBoundingBox();
			onBoundingBoxChanged();
			if (m_rtcGeometry)
				rtcCommitGeometry(m_rtcGeometry);

			m_scene.unlock(SceneInternalModification_Transformation);
			break;

		default:
			Node::fireChanged(paramId);
			break;
	}
}
