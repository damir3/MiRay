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

#include "../../Shared/Interfaces/LightNode.h"
#include "Node.h"
#include "AbstractLight.h"
#include "RenderMath.h"

class DirectionalLight final : public Node, public IDirectionalLight, public AbstractLight
{
	Q_OBJECT
	Q_INTERFACES(IDirectionalLight)

	ColorParameterImpl		m_color;
	ColorIntensityParameter	m_intensity;
	ScalarParameterImpl		m_angularSize;
	ScalarParameterImpl		m_yaw;
	ScalarParameterImpl		m_pitch;
	vec3	m_specularIntensity;
	vec3	m_diffuseIntensity;
	float	m_directPdf;
	Frame	m_frame;
	float	m_cosAng;

public:
	DirectionalLight(const QString & name, Scene & scene, Node * parent);

	eSceneElementType type() const override { return SceneElement_DirectionalLight; }

	ColorParameterImpl & color() override { return m_color; }
	ColorIntensityParameter & intensity() override { return m_intensity; }
	ScalarParameterImpl & angularSize() override { return m_angularSize; }

	ScalarParameterImpl & yaw() override { return m_yaw; }
	ScalarParameterImpl & pitch() override { return m_pitch; }

	void setTransformation(const mat4 & matLocal) override;

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

	bool isFinite() const override { return false; }

	bool isDelta() const override { return false; }

	unsigned geomID() const override { return RTC_INVALID_GEOMETRY_ID; }

	void prepare() override;

	void findLights(std::vector<AbstractLight *> & lights, bool includeGeomLights) override;

	void fireChanged(eParamId paramId) override;
};
