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

#include "../TextureParametersImpl.h"
#include "IndexOfRefractionImpl.h"

class MaterialImpl;
class MaterialGroupImpl;

class TextureRoughnessParameter final : public TextureScalarParameterImpl
{
	Q_OBJECT

protected:
	vec3 realValue(float userValue) const override
	{
		return vec3(sqr(userValue * m_scale));
	}

public:
	explicit TextureRoughnessParameter(float value, eParamId paramId, IParameterOwner & owner)
		: TextureScalarParameterImpl(value, 0.f, 100.f, 1, paramId, owner, 0.01f) {}
};

class CORE_EXPORT MaterialLayerImpl final : public IMaterialLayer, public IParameterOwner
{
	Q_OBJECT

	MaterialImpl &				m_material;
	MaterialGroupImpl *			m_group;

	StringParameterImpl			m_name;
	BooleanParameterImpl		m_enabled;
	TextureScalarParameterImpl	m_mask;
	TextureScalarParameterImpl	m_bump;

	BooleanParameterImpl		m_diffuseLayer;
	TextureColorParameterImpl	m_diffuseColor;
	TextureScalarParameterImpl	m_diffuseOpacity;
	TextureColorParameterImpl	m_diffuseTransmission;
	BooleanParameterImpl		m_diffuseTextureLayerMask;

	BooleanParameterImpl		m_emissiveLayer;
	TextureColorParameterImpl	m_emissiveColor;
	ColorIntensityParameter		m_emissiveIntensity;

	BooleanParameterImpl		m_specularLayer;
	IndexOfRefractionImpl		m_ior;
	TextureColorParameterImpl	m_reflection;
	TextureColorParameterImpl	m_transmission;
	TextureColorParameterImpl	m_reflection90;
	ScalarParameterImpl			m_reflection90Level;
	TextureRoughnessParameter	m_roughness;
	TextureRoughnessParameter	m_anisotropy;
	TextureScalarParameterImpl	m_anisotropyAngle;

	BooleanParameterImpl		m_thinFilmInterference;
	TextureScalarParameterImpl	m_thickness;
	ScalarParameterImpl			m_minThickness;
	IndexOfRefractionImpl		m_iorFilm;

	MaterialLayerImpl *			m_prev;
	MaterialLayerImpl *			m_next;
	bool	m_isEmissive;

public:
	MaterialLayerImpl(MaterialImpl & material, MaterialGroupImpl * group);
	~MaterialLayerImpl() override;

	void setGroup(MaterialGroupImpl * group) { m_group = group; }

	StringParameterImpl & name() override { return m_name; }
	BooleanParameterImpl & enabled() override { return m_enabled; }
	TextureScalarParameterImpl & mask() override { return m_mask; }
	TextureScalarParameterImpl & bump() override { return m_bump; }
	bool hasBump() { return m_bump.texture()->valid() && m_bump.value() != 0.f; }

	BooleanParameterImpl & diffuseLayer() override { return m_diffuseLayer; }
	TextureColorParameterImpl & diffuseColor() override { return m_diffuseColor; }
	TextureScalarParameterImpl & diffuseOpacity() override { return m_diffuseOpacity; }
	TextureColorParameterImpl & diffuseTransmission() override { return m_diffuseTransmission; }
	BooleanParameterImpl & diffuseTextureLayerMask() override { return m_diffuseTextureLayerMask; }

	BooleanParameterImpl & emissiveLayer() override { return m_emissiveLayer; }
	TextureColorParameterImpl & emissiveColor() override { return m_emissiveColor; }
	ColorIntensityParameter & emissiveIntensity() override { return m_emissiveIntensity; }

	BooleanParameterImpl & specularLayer() override { return m_specularLayer; }
	IndexOfRefractionImpl & indexOfRefraction() override { return m_ior; }
	TextureColorParameterImpl & reflection() override { return m_reflection; }
	TextureColorParameterImpl & transmission() override { return m_transmission; }
	TextureColorParameterImpl & reflection90() override { return m_reflection90; }
	ScalarParameterImpl & reflection90Level() override { return m_reflection90Level; }
	TextureScalarParameterImpl & roughness() override { return m_roughness; }
	TextureScalarParameterImpl & anisotropy() override { return m_anisotropy; }
	TextureScalarParameterImpl & anisotropyAngle() override { return m_anisotropyAngle; }

	BooleanParameterImpl & thinFilmInterference() override { return m_thinFilmInterference; }
	TextureScalarParameterImpl & thickness() override { return m_thickness; }
	ScalarParameterImpl & minThickness() override { return m_minThickness; }
	IndexOfRefractionImpl & filmIndexOfRefraction() override { return m_iorFilm; }

	void setNeighbours(MaterialLayerImpl * prev, MaterialLayerImpl * next);
	MaterialLayerImpl * getPrev() const { return m_prev; }
	MaterialLayerImpl * getNext() const { return m_next; }

	MaterialImpl & material() const { return m_material; }
	MaterialGroupImpl * getGroup() const { return m_group; }

	bool isSpecular() const { return m_specularLayer.get(); }
	bool isEmissive() const { return m_isEmissive; }

	void collectFileNames(QSet<QString> &res) const;
	void updateFileNames(const QMap<QString, QString> &map);

	bool _load(const QDomElement & node, const ModelLoadingContext &ctx, uint16_t version);
	bool _load(const QJsonObject & obj, const ModelLoadingContext &ctx);
	void _save(QJsonObject & obj, const ModelSavingContext &ctx) const;

	QByteArray save(const ModelSavingContext &ctx) const override;

	bool compareLayerMaskAndBump(const MaterialLayerImpl & otherLayer) const;
	void copyDiffuseLayer(const MaterialLayerImpl & srcLayer);

	bool calculateReflectionFromMeasuredIOR(vec3 & outReflection, vec3 & outReflection90, float & outReflection90Level, const IndexOfRefractionImpl * ior) const;

	// IParameterOwner
	CoreInstance & core() const override;
	void pushCommand(QUndoCommand *cmd) override;
	void fireChanged(eParamId paramId) override;

	void updateParams(int flags);
};

using MaterialLayerPtr = std::unique_ptr<MaterialLayerImpl>;
using MaterialLayersList = std::vector<MaterialLayerPtr>;
