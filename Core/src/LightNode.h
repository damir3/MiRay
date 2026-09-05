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

class LightNode final : public Node, public ILightNode, public AbstractLight
{
	Q_OBJECT
	Q_INTERFACES(ILightNode)

	ColorParameterImpl		m_color;
	ScalarParameterImpl		m_intensity;
	ScalarParameterImpl		m_radius;
	vec3			m_specularIntensity;
	float			m_geomRadius;
	float			m_invArea;
	RTCGeometry		m_rtcGeometry;
	unsigned int	m_geomID;

	void calculateBoundingBox() override;
	void updateTransformation() final override;

	bool senerateSample(const vec3 & target, float radius, const vec2 & ur, vec3 & pos, vec3 & normal) const;

public:
	LightNode(const QString & name, Scene & scene, Node * parent);
	~LightNode() override;

	eSceneElementType type() const final override { return SceneElement_Light; }

	ColorParameterImpl & color() override { return m_color; }
	ScalarParameterImpl & intensity() override { return m_intensity; }
	ScalarParameterImpl & radius() override { return m_radius; }

	float getGeomRadius() const { return m_geomRadius; }

	vec3 illuminate(const vec3 & aReceivingPosition,
					Sampler & sampler,
					vec3 & oDirectionToLight, float & oDistance,
					float & oDirectPdfW, float * oEmissionPdfW = nullptr,
					float * oCosAtLight = nullptr) const final override;

	vec3 illuminateFloor(const vec3 & aReceivingPosition,
						 Sampler & sampler,
						 vec3 & oDirectionToLight, float & oDistance,
						 float & oDirectPdfW, float * oEmissionPdfW = nullptr,
						 float * oCosAtLight = nullptr) const final override;

	vec3 emitParticle(Sampler & sampler,
					  vec3 & oPosition, vec3 & oDirection,
					  float & oEmissionPdfW, float * oDirectPdfA,
					  float * oCosThetaLight) const final override;

	vec3 getRadiance(const Ray & ray,
					 float * oDirectPdfA = nullptr, float * oEmissionPdfW = nullptr,
					 float * oCosThetaLight = nullptr) const final override;

	bool isFinite() const final override { return true; }

	bool isDelta() const final override { return false; }

	unsigned geomID() const final override { return m_geomID; }

	void prepare() override;

	static LightNode * getByRay(RTCScene scene, const Ray & ray)
	{
		if (auto rtcGeom = rtcGetGeometry(scene, ray.hit.geomID))
			return static_cast<LightNode *>(rtcGetGeometryUserData(rtcGeom));
		return nullptr;
	}

	void createCollisionGeometries() override;
	void destroyCollisionGeometries() override;

	void findLights(std::vector<AbstractLight *> & lights, bool includeGeomLights) override;

	void updateVisibility(bool visible) override;

	void fireChanged(eParamId paramId) override;
};
