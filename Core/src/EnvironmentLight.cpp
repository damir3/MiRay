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

#include "EnvironmentLight.h"
#include "TextureParametersImpl.h"

void EnvironmentLight::prepare()
{
	m_intensity = m_scene.environmentIntensity().rgb();
	m_rotation = glm::rotationYawPitchRoll(glm::radians(m_scene.environmentHorizontalRotation().get()), glm::radians(m_scene.environmentVerticalRotation().get()), 0.f);
}

vec3 EnvironmentLight::doIlluminate(const vec3 & aReceivingPosition,
									Sampler & sampler,
									vec3 & oDirectionToLight, float & oDistance,
									float & oDirectPdfW, float * oEmissionPdfW,
									float * oCosAtLight) const
{
	auto radiance = getRadiance(aReceivingPosition, oDirectionToLight);
	auto & sceneSphere = m_scene.sceneSphere();

	// This stays even with image sampling
	oDistance = sceneSphere.radius * 2.f;

	if (oEmissionPdfW)
		*oEmissionPdfW = oDirectPdfW * concentricDiskPdfA() * sceneSphere.invRadiusSqr;

	if (oCosAtLight)
		*oCosAtLight = 1.f;

	return radiance;
}

vec3 EnvironmentLight::illuminate(const vec3 & aReceivingPosition,
								  Sampler & sampler,
								  vec3 & oDirectionToLight, float & oDistance,
								  float & oDirectPdfW, float * oEmissionPdfW,
								  float * oCosAtLight) const
{
	const auto aRndTuple = sampler.generate2D();
	oDirectionToLight = uniformSampleSphere(aRndTuple.x, aRndTuple.y);
	oDirectPdfW = uniformSpherePdfW();

	return doIlluminate(aReceivingPosition, sampler, oDirectionToLight, oDistance, oDirectPdfW, oEmissionPdfW, oCosAtLight);
}

vec3 EnvironmentLight::illuminateFloor(const vec3 & aReceivingPosition,
									   Sampler & sampler,
									   vec3 & oDirectionToLight, float & oDistance,
									   float & oDirectPdfW, float * oEmissionPdfW,
									   float * oCosAtLight) const
{
	const auto aRndTuple = sampler.generate2D();
	oDirectionToLight = uniformSampleHemisphere(aRndTuple.x, aRndTuple.y);
	oDirectPdfW = uniformHemispherePdfW();

	return doIlluminate(aReceivingPosition, sampler, oDirectionToLight, oDistance, oDirectPdfW, oEmissionPdfW, oCosAtLight);
}

vec3 EnvironmentLight::emitParticle(Sampler & sampler,
									vec3 & oPosition, vec3 & oDirection,
									float & oEmissionPdfW, float * oDirectPdfA,
									float * oCosThetaLight) const
{
	const auto rndDirSamples = sampler.generate2D();
	const auto rndPosSamples = sampler.generate2D();

	oDirection = uniformSampleSphere(rndDirSamples.x, rndDirSamples.y);
	auto directPdf = uniformSpherePdfW();

	// Stays even with image sampling
	auto xy = concentricSampleDisk(rndPosSamples.x, rndPosSamples.y);

	auto dirX = glm::perpendicular(oDirection);
	auto dirY = glm::cross(dirX, oDirection);

	auto & sceneSphere = m_scene.sceneSphere();
	oPosition = sceneSphere.center + sceneSphere.radius * (-oDirection + dirX * xy.x + dirY * xy.y);

	auto radiance = getRadiance(oPosition, -oDirection);

	oEmissionPdfW = directPdf * concentricDiskPdfA() * sceneSphere.invRadiusSqr;

	// For background we lie about Pdf being in area measure
	if (oDirectPdfA)
		*oDirectPdfA = directPdf;

	// Not used for infinite or delta lights
	if (oCosThetaLight)
		*oCosThetaLight = 1.f;

	return radiance;
}

vec3 EnvironmentLight::getRadiance(const Ray & ray,
								   float *oDirectPdfA, float *oEmissionPdfW, float *oCosThetaLight) const
{
	auto directPdf = uniformSpherePdfW();
	auto radiance = getRadiance(ray.origin(), ray.direction());

	if (oDirectPdfA)
		*oDirectPdfA = directPdf;

	if (oEmissionPdfW)
		*oEmissionPdfW = directPdf * concentricDiskPdfA() * m_scene.sceneSphere().invRadiusSqr;

	// Not used for infinite or delta lights
	if (oCosThetaLight)
		*oCosThetaLight = 1.f;

	return radiance;
}

vec3 EnvironmentLight::getRadiance(const vec3 & pos, const vec3 & dir) const
{
	auto radius = m_scene.environmentSize().get();
	if (radius <= 0) {
		auto tc = sphericalTexCoords( glm::transformNormal(dir, m_rotation) );
		return m_scene.environment().getColor(tc) * m_intensity;
	}

	auto delta = pos;
	delta.z -= m_scene.environmentVerticalOffset().get() * radius;
	auto C = glm::length2(delta) - sqr(radius);
	auto B = glm::dot(delta, dir);
	auto D = B * B - C;
	if (D <= 0.f)
		return vec3(0.f);

	auto Q = std::sqrt(D);
	auto t = Q - B;
	auto ip = delta + dir * t;
	auto tc = sphericalTexCoords( glm::transformNormal(ip, m_rotation) );
	return m_scene.environment().getColor(tc) * m_intensity;
}
