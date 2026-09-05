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

#include "DirectionalLight.h"

DirectionalLight::DirectionalLight(const QString & name, Scene & scene, Node * parent)
	: Node(name, scene, parent)
	, m_color(vec3(1.f), PID_DIRECTION_LIGHT_COLOR, *this)
	, m_intensity(50.f, 0.f, FLT_MAX, 2, PID_DIRECTION_LIGHT_INTENSITY, *this)
	, m_angularSize(4.f, 0.f, 180.f, 1, PID_DIRECTION_LIGHT_ANGULAR_SIZE, *this)
	, m_yaw(0.f, -360.f, 360.f, 1, PID_DIRECTION_LIGHT_YAW, *this)
	, m_pitch(0.f, -90.f, 90.f, 1, PID_DIRECTION_LIGHT_PITCH, *this)
	, m_specularIntensity(0.f)
	, m_cosAng(1.f)
{
	m_position.setVisible(false);
	m_rotation.setVisible(false);
	m_scale.setVisible(false);
}

// ------------------------------------------------------------------------ //

vec3 DirectionalLight::illuminate(const vec3 & aReceivingPosition,
								  Sampler & sampler,
								  vec3 & oDirectionToLight, float & oDistance,
								  float & oDirectPdfW, float * oEmissionPdfW,
								  float * oCosAtLight) const
{
	auto & sceneSphere = m_scene.sceneSphere();
	const auto rndDirSamples = sampler.generate2D();

	auto uz = lerp(m_cosAng, 1.f, rndDirSamples.x);
	auto v = uniformSampleHemisphere(uz, rndDirSamples.y);

	oDirectionToLight = (m_frame.tangent() * v.x + m_frame.binormal() * v.y - m_frame.normal() * v.z);
	oDistance = sceneSphere.radius + glm::length(aReceivingPosition - sceneSphere.center);
	oDirectPdfW = m_directPdf;

	if (oCosAtLight)
		*oCosAtLight = 1.f;

	if (oEmissionPdfW)
		*oEmissionPdfW = oDirectPdfW * concentricDiskPdfA() * sceneSphere.invRadiusSqr;

	return m_specularIntensity;
}

vec3 DirectionalLight::illuminateFloor(const vec3 & aReceivingPosition,
									   Sampler & sampler,
									   vec3 & oDirectionToLight, float & oDistance,
									   float & oDirectPdfW, float * oEmissionPdfW,
									   float * oCosAtLight) const
{
	return illuminate(aReceivingPosition, sampler, oDirectionToLight, oDistance, oDirectPdfW, oEmissionPdfW, oCosAtLight);
}

vec3 DirectionalLight::emitParticle(Sampler & sampler,
									vec3 & oPosition, vec3 & oDirection,
									float & oEmissionPdfW, float * oDirectPdfA,
									float * oCosThetaLight) const
{
	const auto rndDirSamples = sampler.generate2D();
	const auto rndPosSamples = sampler.generate2D();

	auto uz = lerp(m_cosAng, 1.f, rndDirSamples.x);
	auto v = uniformSampleHemisphere(uz, rndDirSamples.y);

	oDirection = (m_frame.tangent() * v.x + m_frame.binormal() * v.y + m_frame.normal() * v.z);
	auto dirX = glm::perpendicular(oDirection);
	auto dirY = glm::cross(dirX, oDirection);

	auto xy = concentricSampleDisk(rndPosSamples.x, rndPosSamples.y);

	auto & sceneSphere = m_scene.sceneSphere();
	oPosition = sceneSphere.center + sceneSphere.radius * (-oDirection + dirX * xy.x + dirY * xy.y);

	oEmissionPdfW = m_directPdf * concentricDiskPdfA() * sceneSphere.invRadiusSqr;

	if (oDirectPdfA)
		*oDirectPdfA = m_directPdf;

	// Not used for infinite or delta lights
	if (oCosThetaLight)
		*oCosThetaLight = 1.f;

	return m_specularIntensity;
}

vec3 DirectionalLight::getRadiance(const Ray & ray,
								   float * oDirectPdfA, float * oEmissionPdfW,
								   float * oCosThetaLight) const
{
	auto dp = -glm::dot(m_frame.normal(), ray.direction());
	if (dp < m_cosAng)
		return vec3(0.f);

	if (oDirectPdfA)
		*oDirectPdfA = m_directPdf;

	// Not used for infinite or delta lights
	if (oCosThetaLight)
		*oCosThetaLight = 1.f;

	if (oEmissionPdfW)
		*oEmissionPdfW = m_directPdf * concentricDiskPdfA() * m_scene.sceneSphere().invRadiusSqr;

	return m_specularIntensity;
}

// ------------------------------------------------------------------------ //

void DirectionalLight::prepare()
{
	m_frame.setFromZ(-glm::axisX(m_matGlobal));
	m_cosAng = std::cos(glm::radians(std::max(m_angularSize.get() * 0.5f, 0.1f)));
	m_directPdf = uniformConePdf(m_cosAng);
	m_specularIntensity = m_color.rgb() * (m_intensity.rgb() / (1.f - sqr(m_cosAng)));
}

void DirectionalLight::findLights(std::vector<AbstractLight *> & lights, bool includeGeomLights)
{
	if (!isBlack(m_color.rgb() * m_intensity.rgb()))
		lights.push_back(this);

	Node::findLights(lights, includeGeomLights);
}

void DirectionalLight::setTransformation(const mat4 & matLocal)
{
	Node::setTransformation(matLocal);

	auto rotation = m_rotation.get();
	m_yaw._set(rotation.z);
	m_pitch._set(rotation.y);
}

// ------------------------------------------------------------------------ //

void DirectionalLight::fireChanged(eParamId paramId)
{
	switch (paramId) {
		case PID_DIRECTION_LIGHT_COLOR:
		case PID_DIRECTION_LIGHT_INTENSITY:
		case PID_DIRECTION_LIGHT_ANGULAR_SIZE:
			break;
		case PID_DIRECTION_LIGHT_YAW:
		case PID_DIRECTION_LIGHT_PITCH:
			m_rotation._set(vec3(0.f, m_pitch.get(), m_yaw.get()));
			Node::fireChanged(PID_NODE_ROTATION);
			break;
		default:
			Node::fireChanged(paramId);
			break;
	}
}
