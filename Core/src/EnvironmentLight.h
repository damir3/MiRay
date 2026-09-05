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

class EnvironmentLight final : public AbstractLight
{
	Scene & m_scene;
	vec3	m_intensity;
	mat4	m_rotation;

	vec3 getRadiance(const vec3 & pos, const vec3 & dir) const;

	vec3 doIlluminate(const vec3 & aReceivingPosition,
					  Sampler & sampler,
					  vec3 & oDirectionToLight, float & oDistance,
					  float & oDirectPdfW, float * oEmissionPdfW,
					  float * oCosAtLight) const;

public:
	EnvironmentLight(Scene & scene) : m_scene(scene) {}

	vec3 illuminate(const vec3 & aReceivingPosition,
					Sampler & sampler,
					vec3 & oDirectionToLight, float & oDistance,
					float & oDirectPdfW, float * oEmissionPdfW,
					float * oCosAtLight) const override;

	vec3 illuminateFloor(const vec3 & aReceivingPosition,
						 Sampler & sampler,
						 vec3 & oDirectionToLight, float & oDistance,
						 float & oDirectPdfW, float * oEmissionPdfW,
						 float * oCosAtLight) const override;

	vec3 emitParticle(Sampler & sampler,
					  vec3 & oPosition, vec3 & oDirection,
					  float & oEmissionPdfW, float * oDirectPdfA,
					  float * oCosThetaLight) const override;

	vec3 getRadiance(const Ray & ray,
					 float * oDirectPdfA = nullptr, float * oEmissionPdfW = nullptr,
					 float * oCosThetaLight = nullptr) const override;

	// Whether the light has a finite extent (area, point) or not (directional, env. map)
	bool isFinite() const override { return false; }

	// Whether the light has delta function (point, directional) or not (area)
	bool isDelta() const override { return false; }

	unsigned geomID() const override { return RTC_INVALID_GEOMETRY_ID; }

	void prepare() override;
};
