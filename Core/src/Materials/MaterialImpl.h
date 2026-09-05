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

#include "MaterialGroupImpl.h"
#include "../ParametersImpl.h"
#include <limits>

enum RaySamplingFlags {
	kOriginInMedium	= 0x0001,
	kEndInMedium	= 0x0002,
};

class CORE_EXPORT MaterialImpl final : public IMaterial, public IParameterOwner, public IStringParameterValidator
{
	Q_OBJECT

	class MaterialManager &		m_owner;
	QUuid						m_guid;
	StringParameterImpl			m_name;
	TextureScalarParameterImpl	m_bump;
	EnumParameterImpl			m_medium;
	IndexOfRefractionImpl		m_ior;
	ColorParameterImpl			m_absorptionColor;
	ScalarParameterImpl			m_absorptionAttenuation;
	ColorParameterImpl			m_emissionColor;
	ScalarParameterImpl			m_emissionScale;
	BooleanParameterImpl		m_subsurfaceScattering;
	ColorParameterImpl			m_scatteringColor;
	ScalarParameterImpl			m_scatteringScale;
	ScalarParameterImpl			m_scatteringAsymmetry;
	IntegerParameterImpl		m_priority;
	BooleanParameterImpl		m_doubleSided;

	MaterialGroupsList m_groups;

	vec3				m_absorptionCoef = vec3(0.f);
	vec3				m_emissionCoef = vec3(0.f);
	vec3				m_scatteringCoef = vec3(0.f);
	vec3				m_attenuationCoef = vec3(0.f);

	float				m_minAttenuationCoef = 0.f;
	int					m_minAttenuationCoefIndex = 0;
	float				m_maxBeamLength = std::numeric_limits<float>::infinity();
	float				m_meanFreePath = std::numeric_limits<float>::infinity();

	bool				m_hasAbsorption = false;
	bool				m_hasScattering = false;
	bool				m_hasAttenuation = false;

	vec3				m_uniqueColor = vec3(0.f);

	bool				m_validPreview = false;
	QImage				m_preview;
	vec3				m_previewColor = vec3(1.f);
	const TextureImpl *	m_previewTexture = nullptr;
	bool				m_previewEmission = false;
	vec3				m_previewEmissiveColor = vec3(1.f);
	const TextureImpl *	m_previewEmissiveTexture = nullptr;

	void updatePreviewColor();
	void updateMediumCoefs(eParamId);

	void updateParams(eParamId);

	QString validate(IStringParameter *property, QString val) const override;

public:
	MaterialImpl(MaterialManager & owner, const QString & name);
	~MaterialImpl() override;

	MaterialManager & owner() const { return m_owner; }

	bool load(const QByteArray &data, const ModelLoadingContext &ctx);
	QByteArray save(const ModelSavingContext &ctx) const override;

	void update() override;

	bool _loadParams(const QJsonObject & params, const ModelLoadingContext &ctx);
	bool _load(const QDomElement & node, const ModelLoadingContext &ctx);
	bool _load(const QJsonObject & obj, const ModelLoadingContext &ctx);
	void _save(QJsonObject & obj, const ModelSavingContext &ctx) const;

	void collectFileNames(QSet<QString> &res) const;
	void updateFileNames(const QMap<QString, QString>& map);

	QString guid() const override { return m_guid.isNull() ? QString() : m_guid.toString(); }
	const QUuid & _guid() const { return m_guid; }
	void _setGuid(const QUuid & guid) { m_guid = guid; }

	StringParameterImpl & name() override { return m_name; }
	const StringParameterImpl & name() const override { return m_name; }

	TextureScalarParameterImpl & bump() override { return m_bump; }
	bool hasBump() { return m_bump.texture()->valid() && m_bump.value() != 0.f; }

	EnumParameterImpl & medium() override { return m_medium; }

	IndexOfRefractionImpl & indexOfRefraction() override { return m_ior; }

	ColorParameterImpl & absorptionColor() override { return m_absorptionColor; }
	ScalarParameterImpl & absorptionAttenuation() override { return m_absorptionAttenuation; }

	ColorParameterImpl & emissionColor() override { return m_emissionColor; }
	ScalarParameterImpl & emissionScale() override { return m_emissionScale; }

	BooleanParameterImpl & subsurfaceScattering() override { return m_subsurfaceScattering; }
	ColorParameterImpl & scatteringColor() override { return m_scatteringColor; }
	ScalarParameterImpl & scatteringScale() override { return m_scatteringScale; }
	ScalarParameterImpl & scatteringAsymmetry() override { return m_scatteringAsymmetry; }

	IntegerParameterImpl & priority() override { return m_priority; }

	BooleanParameterImpl & doubleSided() override { return m_doubleSided; }

	bool isHomogeneous() const { return true; }

	bool isEmpty() const { return m_medium.getIndex() == MEDIUM_TYPE_NONE; }

	bool isOpaque() const
	{
		auto mediumType = m_medium.getIndex();
		return (mediumType == MEDIUM_TYPE_OPAQUE) || (mediumType == MEDIUM_TYPE_MEASURED && m_ior.attenuation() == 0.f);
	}

	bool hasAbsorption() const { return m_hasAbsorption; }
	const vec3 & getAbsorptionCoef() const { return m_absorptionCoef; }

	bool hasScattering() const { return m_hasScattering; }
	const vec3 & getScatteringCoef() const { return m_scatteringCoef; }

	bool hasAttenuation() const { return m_hasAttenuation; }
	const vec3 & getAttenuationCoef() const { return m_attenuationCoef; }

	int getMediumBoundaryPriority() const { return m_priority.get(); }

	float mMinPositiveAttenuationCoefComp() const { return m_minAttenuationCoef; }
	int mMinPositiveAttenuationCoefCompIndex() const { return m_minAttenuationCoefIndex; }

	float continuationProbability() const { return 1.f; }
	float meanCosine() const { return m_scatteringAsymmetry.get(); }

	float maxBeamLength() const { return m_maxBeamLength; }

	float getMeanFreePath(const vec3 &/*aPos*/) const { return m_meanFreePath; }
	float getMeanFreePath() const { return m_meanFreePath; }

	vec3 evalAttenuation(const float dist) const;

	vec3 evalAttenuation(const float distMin, const float distMax) const
	{
		return evalAttenuation(distMax - distMin);
	}

	vec3 evalEmission(const float distMin, const float distMax) const
	{
		return m_emissionCoef * (distMax - distMin);
	}

	vec3 getPreviewColor(const Ray & ray) const;

	const vec3 & uniqueColor() const { return m_uniqueColor; }

	// Samples the medium along the given ray starting at its origin.
	// Returns distance along ray to sampled point in media or distance to boundary if sample fell behind.
	float sampleRay(const float aDistToBoundary,
					const float aRandom,
					float * oPdf,
					const uint32_t aRaySamplingFlags = 0,
					float * oRevPdf = nullptr) const;

	// Get PDF (and optionally reverse PDF) of sampling in the medium along the given ray.
	// Sampling starts at the given min distance and ends at the max distance.
	// If end is said to be inside the medium, PDF for sampling in medium is returned, otherwise PDF for sampling behind the medium is returned.
	float raySamplePdf(const float aDistMin,
					   const float aDistMax,
					   const uint32_t aRaySamplingFlags = 0,
					   float * oRevPdf = nullptr) const;

	size_t numGroups() const override { return m_groups.size(); }
	MaterialGroupImpl *group(size_t i) const override { return i < m_groups.size() ? m_groups[i].get() : nullptr; }
	const decltype(m_groups) & groups() const { return m_groups; }

	MaterialGroupImpl *addGroup(size_t i) override;
	void removeGroup(size_t i) override;
	void moveGroup(size_t from, size_t to) override;

	void _addGroup(MaterialGroupPtr group, size_t pos);
	MaterialGroupPtr _removeGroup(size_t pos);
	IMaterialGroup * _createGroup(size_t pos) override;

	void moveLayer(size_t groupFrom, size_t layerFrom, size_t groupTo, size_t layerTo) override;

	// IParameterOwner
	CoreInstance & core() const override;
	void pushCommand(QUndoCommand *cmd) override;
	void fireChanged(eParamId paramId) override;

	// Preview
	bool isPreviewValid() const override;
	QImage getPreview() const override;
	void setPreview(QImage image) override;

	static bool getMaterialInformation(const QByteArray &data, IApplicationContext::MaterialInformation &info);
};

using MaterialPtr = std::unique_ptr<MaterialImpl>;
using MaterialsList = std::vector<MaterialPtr>;
