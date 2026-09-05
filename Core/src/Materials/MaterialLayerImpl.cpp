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

#include "MaterialLayerImpl.h"
#include "MaterialImpl.h"
#include "../../Shared/Interfaces/SerializationContext.h"

enum eLayerType {
	LAYER_TYPE_DIFFUSE = 0,
	LAYER_TYPE_FRESNEL,
	LAYER_TYPE_MEASURED_DATA,
};

static const char * const LAYER_TYPES[] = {"Diffuse", "Fresnel", "Measured Data", nullptr};

MaterialLayerImpl::MaterialLayerImpl(MaterialImpl & material, MaterialGroupImpl * group)
	: m_material(material)
	, m_group(group)
	, m_name("Layer", PID_LAYER_NAME, *this, nullptr)
	, m_enabled(true, PID_LAYER_ENABLED, *this)
	, m_mask(100.f, 0.f, 100.f, 1, PID_LAYER_MASK, *this, 0.01f)
	, m_bump(0.f, 0.f, 100.f, 3, PID_LAYER_BUMP, *this, 1.f, TextureImpl::TYPE_BUMP_MAP) // cm
	, m_diffuseLayer(true, PID_LAYER_DIFFUSE_LAYER, *this)
	, m_diffuseColor(vec3(1.f), PID_LAYER_DIFFUSE_COLOR, *this)
	, m_diffuseOpacity(100.f, 0.f, 100.f, 1, PID_LAYER_DIFFUSE_OPACITY, *this, 0.01f)
	, m_diffuseTransmission(vec3(1.f), PID_LAYER_DIFFUSE_TRANSMISSION, *this)
	, m_diffuseTextureLayerMask(false, PID_LAYER_DIFFUSE_TEXTURE_LAYER_MASK, *this)
	, m_emissiveLayer(false, PID_LAYER_EMISSIVE_LAYER, *this)
	, m_emissiveColor(vec3(1.f), PID_LAYER_EMISSIVE_COLOR, *this)
	, m_emissiveIntensity(100.f, 0.f, FLT_MAX, 1, PID_LAYER_EMISSIVE_LEVEL, *this)
	, m_specularLayer(false, PID_LAYER_SPECULAR_LAYER, *this)
	, m_reflection(vec3(1.f), PID_LAYER_REFLECTION, *this)
	, m_transmission(vec3(1.f), PID_LAYER_TRANSMISSION, *this)
	, m_ior(PID_LAYER_IOR, 1.5f, *this)
	, m_reflection90Level(0.f, 0.f, 100.f, 1, PID_LAYER_REFLECTION_90_LEVEL, *this, 0.01f)
	, m_reflection90(vec3(1.f), PID_LAYER_REFLECTION_90, *this)
	, m_roughness(0.f, PID_LAYER_ROUGHNESS, *this)
	, m_anisotropy(0.f, PID_LAYER_ANISOTROPY, *this)
	, m_anisotropyAngle(0.f, -180.f, 180.f, 1, PID_LAYER_ANISOTROPY_ANGLE, *this) // degrees
	, m_thinFilmInterference(false, PID_LAYER_THIN_FILM_INTERFERENCE, *this)
	, m_thickness(200.f, 0.f, 2000.f, 0, PID_LAYER_FILM_THICKNESS, *this) // nm
	, m_minThickness(100.f, 1.f, 2000.f, 0, PID_LAYER_MIN_FILM_THICKNESS, *this) // nm
	, m_iorFilm(PID_LAYER_FILM_IOR, 2.7f, *this)
	, m_prev(nullptr)
	, m_next(nullptr)
	, m_isEmissive(false)
{
	updateParams(-1);
}

MaterialLayerImpl::~MaterialLayerImpl()
{
}

// ------------------------------------------------------------------------ //

void MaterialLayerImpl::setNeighbours(MaterialLayerImpl * prev, MaterialLayerImpl * next)
{
	m_prev = prev;
	m_next = next;
}

// ------------------------------------------------------------------------ //

#define MATERIAL_LAYER_NAME						"name"
#define MATERIAL_LAYER_ENABLE					"enable"
#define MATERIAL_LAYER_TYPE						"type"
#define MATERIAL_LAYER_MASK						"mask"
#define MATERIAL_LAYER_BUMP						"bump"

#define MATERIAL_LAYER_DIFFUSE					"diffuse"
#define MATERIAL_LAYER_DIFFUSE_COLOR			"color"
#define MATERIAL_LAYER_DIFFUSE_OPACITY			"opacity"
#define MATERIAL_LAYER_DIFFUSE_TRANSMISSION		"transmission"
#define MATERIAL_LAYER_DIFFUSE_ALPHA_MASK		"alpha-layer-mask"

#define MATERIAL_LAYER_EMISSIVE					"emissive"
#define MATERIAL_LAYER_EMISSIVE_COLOR			"color"
#define MATERIAL_LAYER_EMISSIVE_INTENSITY		"intensity"

#define MATERIAL_LAYER_SPECULAR					"specular"
#define MATERIAL_LAYER_REFLECTION				"reflection"
#define MATERIAL_LAYER_REFLECTION_90			"reflection-90"
#define MATERIAL_LAYER_REFLECTION_90_LEVEL		"reflection-90-level"
#define MATERIAL_LAYER_TRANSMISSION				"transmission"
#define MATERIAL_LAYER_OPACITY					"opacity"
#define MATERIAL_LAYER_ROUGHNESS				"roughness"
#define MATERIAL_LAYER_ANISOTROPY				"anisotropy"
#define MATERIAL_LAYER_ANISOTROPY_ANGLE			"anisotropy-angle"

#define MATERIAL_LAYER_THIN_FILM				"thin-film"
#define MATERIAL_LAYER_THIN_FILM_ENABLE			"enable"
#define MATERIAL_LAYER_FILM_THICKNESS			"thickness"
#define MATERIAL_LAYER_MIN_FILM_THICKNESS		"min-thickness"

inline int LayerParamFlag(eParamId id)
{
	return (1 << (id - PID_LAYER));
}

void MaterialLayerImpl::updateParams(int flags)
{
	if (flags & (LayerParamFlag(PID_LAYER_DIFFUSE_LAYER) | LayerParamFlag(PID_LAYER_DIFFUSE_OPACITY))) {
		const bool enabled = m_diffuseLayer.get();
		m_diffuseColor.setEnabled(enabled);
		m_diffuseColor.setVisible(enabled);
		m_diffuseOpacity.setEnabled(enabled);
		m_diffuseOpacity.setVisible(enabled);
		m_diffuseTransmission.setEnabled(enabled && m_diffuseOpacity.value() < 1.f);
		m_diffuseTransmission.setVisible(enabled);
		m_diffuseTextureLayerMask.setVisible(enabled);
	}

	if (flags & (LayerParamFlag(PID_LAYER_DIFFUSE_LAYER) | LayerParamFlag(PID_LAYER_DIFFUSE_COLOR)))
		m_diffuseTextureLayerMask.setEnabled(m_diffuseTextureLayerMask.isVisible() && m_diffuseColor.texture()->isTransparent());

	if (flags & (LayerParamFlag(PID_LAYER_EMISSIVE_LAYER) | LayerParamFlag(PID_LAYER_EMISSIVE_LEVEL))) {
		const bool enabled = m_emissiveLayer.get();
		m_emissiveColor.setEnabled(enabled && !isBlack(m_emissiveIntensity.rgb()));
		m_emissiveIntensity.setEnabled(enabled);
		m_emissiveColor.setVisible(enabled);
		m_emissiveIntensity.setVisible(enabled);
	}

	if (flags & (LayerParamFlag(PID_LAYER_SPECULAR_LAYER) | LayerParamFlag(PID_LAYER_IOR))) {
		const auto enabled = m_specularLayer.get();
		const auto measured = m_ior.iorType() == IORType::MEASURED;
		m_ior.setEnabled(enabled);
		m_ior.setVisible(enabled);

		m_reflection.setEnabled(!measured);
		m_reflection.setVisible(enabled && !measured);
		m_transmission.setEnabled(!measured);
		m_transmission.setVisible(enabled && !measured);

		m_reflection90Level.setEnabled(!measured);
		m_reflection90Level.setVisible(enabled && !measured);
		m_reflection90.setEnabled(enabled && !measured && m_reflection90Level.value() > 0.f);
		m_reflection90.setVisible(enabled && !measured);

		m_roughness.setVisible(enabled);
		m_anisotropy.setEnabled(enabled);
		m_anisotropy.setVisible(enabled);
		m_anisotropyAngle.setEnabled(enabled && m_anisotropy.value() > 0.f);
		m_anisotropyAngle.setVisible(enabled);

		m_thinFilmInterference.setEnabled(enabled);
		m_thinFilmInterference.setVisible(enabled);
		auto visible = m_thinFilmInterference.isVisible() && m_thinFilmInterference.get();
		m_thickness.setVisible(visible);
		m_minThickness.setVisible(visible);
		m_iorFilm.setVisible(visible);
	} else {
		if (flags & LayerParamFlag(PID_LAYER_REFLECTION_90_LEVEL))
			m_reflection90.setEnabled(m_reflection90Level.isEnabled() && m_reflection90Level.value() > 0.f);

		if (flags & LayerParamFlag(PID_LAYER_ANISOTROPY))
			m_anisotropyAngle.setEnabled(m_anisotropy.isEnabled() && m_anisotropy.value() > 0.f);
	}

	if (flags & LayerParamFlag(PID_LAYER_THIN_FILM_INTERFERENCE)) {
		auto enable = m_thinFilmInterference.isEnabled() && m_thinFilmInterference.get();
		m_thickness.setEnabled(enable);
		m_iorFilm.setEnabled(enable);
		enable &= m_thickness.texture()->enabled().get() && !m_thickness.texture()->isEmpty();
		m_minThickness.setEnabled(enable);

		auto visible = m_thinFilmInterference.isVisible() && m_thinFilmInterference.get();
		m_thickness.setVisible(visible);
		m_minThickness.setVisible(visible);
		m_iorFilm.setVisible(visible);
	} else if (flags & LayerParamFlag(PID_LAYER_FILM_THICKNESS)) {
		auto enable = m_thinFilmInterference.get() && m_thickness.texture()->enabled().get() && !m_thickness.texture()->isEmpty();
		m_minThickness.setEnabled(enable);
	}

	m_isEmissive = m_emissiveLayer.get() && maxComponent(m_emissiveColor.rgb() * m_emissiveIntensity.rgb()) > 2.f &&
		!(m_specularLayer.get() && isBlack(m_transmission.rgb()));
}

bool MaterialLayerImpl::_load(const QDomElement & node, const ModelLoadingContext &ctx, uint16_t version)
{
	m_name._set(node.attribute(MATERIAL_LAYER_NAME));
	m_enabled.load(node, MATERIAL_LAYER_ENABLE);
	m_mask.load(node, MATERIAL_LAYER_MASK, ctx);
	m_bump.load(node, MATERIAL_LAYER_BUMP, ctx);

	EnumParameterImpl m_type(LAYER_TYPE_DIFFUSE, LAYER_TYPES, PID_LAYER_TYPE, *this);
	if (m_type.load(node, MATERIAL_LAYER_TYPE)) { // old materials
		m_diffuseLayer._set(m_type.getIndex() == LAYER_TYPE_DIFFUSE);
		m_specularLayer._set(m_type.getIndex() >= LAYER_TYPE_FRESNEL);

		if (m_diffuseLayer.get()) {
			m_diffuseColor.load(node, MATERIAL_LAYER_REFLECTION, ctx);
			m_diffuseOpacity.load(node, MATERIAL_LAYER_OPACITY, ctx);
			m_diffuseTransmission.load(node, MATERIAL_LAYER_TRANSMISSION, ctx);
		}

		if (m_specularLayer.get()) {
			m_ior.load(node, ctx);
			m_ior.type()._setIndex(m_type.getIndex() == LAYER_TYPE_MEASURED_DATA ? (int)IORType::MEASURED : (int)IORType::COMPLEX);
			m_reflection.load(node, MATERIAL_LAYER_REFLECTION, ctx);
			m_reflection90.load(node, MATERIAL_LAYER_REFLECTION_90, ctx);
			m_transmission.load(node, MATERIAL_LAYER_TRANSMISSION, ctx);
			m_reflection90Level.load(node, MATERIAL_LAYER_REFLECTION_90_LEVEL);
			m_roughness.load(node, MATERIAL_LAYER_ROUGHNESS, ctx);
			m_anisotropy.load(node, MATERIAL_LAYER_ANISOTROPY, ctx);
			m_anisotropyAngle.load(node, MATERIAL_LAYER_ANISOTROPY_ANGLE, ctx);

			auto filmNode = node.firstChildElement(MATERIAL_LAYER_THIN_FILM);
			if (!filmNode.isNull()) {
				m_thinFilmInterference.load(filmNode, MATERIAL_LAYER_THIN_FILM_ENABLE);
				m_iorFilm.load(filmNode, ctx);
				m_thickness.load(filmNode, MATERIAL_LAYER_FILM_THICKNESS, ctx);
				m_minThickness.load(filmNode, MATERIAL_LAYER_MIN_FILM_THICKNESS);
			}
		}
	} else {
		m_emissiveLayer._set(false);
		m_diffuseLayer._set(false);
		m_specularLayer._set(false);

		auto diffuseNode = node.firstChildElement(MATERIAL_LAYER_DIFFUSE);
		if (!diffuseNode.isNull()) {
			if (!m_diffuseLayer.load(diffuseNode, MATERIAL_LAYER_ENABLE))
				m_diffuseLayer._set(true);

			m_diffuseColor.load(diffuseNode, MATERIAL_LAYER_DIFFUSE_COLOR, ctx);
			m_diffuseOpacity.load(diffuseNode, MATERIAL_LAYER_DIFFUSE_OPACITY, ctx);
			m_diffuseTransmission.load(diffuseNode, MATERIAL_LAYER_DIFFUSE_TRANSMISSION, ctx);
			if (version >= 0x0107)
				m_diffuseTextureLayerMask.load(node, MATERIAL_LAYER_DIFFUSE_ALPHA_MASK);
			else
				m_diffuseTextureLayerMask._set(true);
		}

		auto specularNode = node.firstChildElement(MATERIAL_LAYER_SPECULAR);
		if (!specularNode.isNull()) {
			if (!m_specularLayer.load(specularNode, MATERIAL_LAYER_ENABLE))
				m_specularLayer._set(true);

			m_ior.load(specularNode, ctx);
			m_reflection.load(specularNode, MATERIAL_LAYER_REFLECTION, ctx);
			m_transmission.load(specularNode, MATERIAL_LAYER_TRANSMISSION, ctx);
			m_reflection90Level.load(specularNode, MATERIAL_LAYER_REFLECTION_90_LEVEL);
			m_reflection90.load(specularNode, MATERIAL_LAYER_REFLECTION_90, ctx);
			m_roughness.load(specularNode, MATERIAL_LAYER_ROUGHNESS, ctx);
			m_anisotropy.load(specularNode, MATERIAL_LAYER_ANISOTROPY, ctx);
			m_anisotropyAngle.load(specularNode, MATERIAL_LAYER_ANISOTROPY_ANGLE, ctx);

			auto filmNode = specularNode.firstChildElement(MATERIAL_LAYER_THIN_FILM);
			if (!filmNode.isNull()) {
				if (!m_thinFilmInterference.load(filmNode, MATERIAL_LAYER_THIN_FILM_ENABLE))
					m_thinFilmInterference._set(true);
				m_iorFilm.load(filmNode, ctx);
				m_thickness.load(filmNode, MATERIAL_LAYER_FILM_THICKNESS, ctx);
				m_minThickness.load(filmNode, MATERIAL_LAYER_MIN_FILM_THICKNESS);
			}
		}

		auto emissiveNode = node.firstChildElement(MATERIAL_LAYER_EMISSIVE);
		if (!emissiveNode.isNull()) {
			if (!m_emissiveLayer.load(emissiveNode, MATERIAL_LAYER_ENABLE))
				m_emissiveLayer._set(true);

			m_emissiveColor.load(emissiveNode, MATERIAL_LAYER_EMISSIVE_COLOR, ctx);
			m_emissiveIntensity.load(emissiveNode, MATERIAL_LAYER_EMISSIVE_INTENSITY);
			if (version < 0x0106)
				m_emissiveIntensity._set(ColorUtils::sRGBToLinear(m_emissiveIntensity.get() * 0.01f) * 100.f);
		}
	}

	if (version < 0x0107) {
		m_roughness._set(std::sqrt(m_roughness.get() * 0.01f) * 100.f);
		m_anisotropy._set(std::sqrt(m_anisotropy.get() * 0.01f) * 100.f);

		if (m_diffuseOpacity.get() < 100.f) {
			m_diffuseTransmission._set(m_diffuseTransmission.get() * m_diffuseColor.get());
			if (!m_diffuseColor.texture()->isEmpty()) {
				if (m_diffuseTransmission.texture()->isEmpty())
					m_diffuseTransmission.texture()->copyFrom(*m_diffuseColor.texture());
				else
					LogWarning() << QString("Transmission and diffuse textures conflict in material \"%1\", layer \"%2\"").arg(m_material.name().get()).arg(m_name.get());
			}
		}
	}

	updateParams(-1);

	return true;
}

bool MaterialLayerImpl::_load(const QJsonObject & obj, const ModelLoadingContext &ctx)
{
	m_name._set(obj.value(MATERIAL_LAYER_NAME).toString());
	m_enabled.load(obj, MATERIAL_LAYER_ENABLE);
	m_mask.load(obj, MATERIAL_LAYER_MASK, ctx);
	m_bump.load(obj, MATERIAL_LAYER_BUMP, ctx);

	m_emissiveLayer._set(false);
	m_diffuseLayer._set(false);
	m_specularLayer._set(false);

	if (obj.contains(MATERIAL_LAYER_DIFFUSE)) {
		QJsonObject diffuseObj = obj[MATERIAL_LAYER_DIFFUSE].toObject();
		if (!m_diffuseLayer.load(diffuseObj, MATERIAL_LAYER_ENABLE))
			m_diffuseLayer._set(true);

		m_diffuseColor.load(diffuseObj, MATERIAL_LAYER_DIFFUSE_COLOR, ctx);
		m_diffuseOpacity.load(diffuseObj, MATERIAL_LAYER_DIFFUSE_OPACITY, ctx);
		m_diffuseTransmission.load(diffuseObj, MATERIAL_LAYER_DIFFUSE_TRANSMISSION, ctx);
		if (!m_diffuseTextureLayerMask.load(obj, MATERIAL_LAYER_DIFFUSE_ALPHA_MASK))
			m_diffuseTextureLayerMask._set(true);
	}

	if (obj.contains(MATERIAL_LAYER_SPECULAR)) {
		QJsonObject specularObj = obj[MATERIAL_LAYER_SPECULAR].toObject();
		if (!m_specularLayer.load(specularObj, MATERIAL_LAYER_ENABLE))
			m_specularLayer._set(true);

		m_ior.load(specularObj, ctx);
		m_reflection.load(specularObj, MATERIAL_LAYER_REFLECTION, ctx);
		m_transmission.load(specularObj, MATERIAL_LAYER_TRANSMISSION, ctx);
		m_reflection90Level.load(specularObj, MATERIAL_LAYER_REFLECTION_90_LEVEL);
		m_reflection90.load(specularObj, MATERIAL_LAYER_REFLECTION_90, ctx);
		m_roughness.load(specularObj, MATERIAL_LAYER_ROUGHNESS, ctx);
		m_anisotropy.load(specularObj, MATERIAL_LAYER_ANISOTROPY, ctx);
		m_anisotropyAngle.load(specularObj, MATERIAL_LAYER_ANISOTROPY_ANGLE, ctx);

		if (specularObj.contains(MATERIAL_LAYER_THIN_FILM)) {
			QJsonObject filmObj = specularObj[MATERIAL_LAYER_THIN_FILM].toObject();
			if (!m_thinFilmInterference.load(filmObj, MATERIAL_LAYER_THIN_FILM_ENABLE))
				m_thinFilmInterference._set(true);
			m_iorFilm.load(filmObj, ctx);
			m_thickness.load(filmObj, MATERIAL_LAYER_FILM_THICKNESS, ctx);
			m_minThickness.load(filmObj, MATERIAL_LAYER_MIN_FILM_THICKNESS);
		}
	}

	if (obj.contains(MATERIAL_LAYER_EMISSIVE)) {
		QJsonObject emissiveObj = obj[MATERIAL_LAYER_EMISSIVE].toObject();
		if (!m_emissiveLayer.load(emissiveObj, MATERIAL_LAYER_ENABLE))
			m_emissiveLayer._set(true);

		m_emissiveColor.load(emissiveObj, MATERIAL_LAYER_EMISSIVE_COLOR, ctx);
		m_emissiveIntensity.load(emissiveObj, MATERIAL_LAYER_EMISSIVE_INTENSITY);
	}

	updateParams(-1);
	return true;
}

void MaterialLayerImpl::_save(QJsonObject & obj, const ModelSavingContext &ctx) const
{
	obj[MATERIAL_LAYER_NAME] = m_name.get();

	if (!m_enabled.get())
		m_enabled.save(obj, MATERIAL_LAYER_ENABLE);
	if (!m_mask.isDefault())
		m_mask.save(obj, MATERIAL_LAYER_MASK, ctx);
	if (!m_bump.isDefault())
		m_bump.save(obj, MATERIAL_LAYER_BUMP, ctx);

	if (m_diffuseLayer.get() || !m_diffuseColor.isDefault() || !m_diffuseOpacity.isDefault() || !m_diffuseTransmission.isDefault()) {
		QJsonObject diffuseObj;
		if (!m_diffuseLayer.get())
			m_diffuseLayer.save(diffuseObj, MATERIAL_LAYER_ENABLE);
		m_diffuseColor.save(diffuseObj, MATERIAL_LAYER_DIFFUSE_COLOR, ctx);
		m_diffuseOpacity.save(diffuseObj, MATERIAL_LAYER_DIFFUSE_OPACITY, ctx);
		m_diffuseTransmission.save(diffuseObj, MATERIAL_LAYER_DIFFUSE_TRANSMISSION, ctx);
		obj[MATERIAL_LAYER_DIFFUSE] = diffuseObj;
		m_diffuseTextureLayerMask.save(obj, MATERIAL_LAYER_DIFFUSE_ALPHA_MASK);
	}

	if (m_emissiveLayer.get() || !m_emissiveColor.isDefault() || !m_emissiveIntensity.isDefault()) {
		QJsonObject emissiveObj;
		if (!m_emissiveLayer.get())
			m_emissiveLayer.save(emissiveObj, MATERIAL_LAYER_ENABLE);
		m_emissiveColor.save(emissiveObj, MATERIAL_LAYER_EMISSIVE_COLOR, ctx);
		m_emissiveIntensity.save(emissiveObj, MATERIAL_LAYER_EMISSIVE_INTENSITY);
		obj[MATERIAL_LAYER_EMISSIVE] = emissiveObj;
	}

	if (m_specularLayer.get() || !m_ior.isDefault() || m_thinFilmInterference.get() ||
		!m_reflection.isDefault() || !m_transmission.isDefault() || !m_reflection90Level.isDefault() || !m_reflection90.isDefault() ||
		!m_roughness.isDefault() || !m_anisotropy.isDefault() || !m_anisotropyAngle.isDefault()) {
		QJsonObject specularObj;

		if (!m_specularLayer.get())
			m_specularLayer.save(specularObj, MATERIAL_LAYER_ENABLE);
		m_ior.save(specularObj, ctx);
		m_reflection.save(specularObj, MATERIAL_LAYER_REFLECTION, ctx);
		m_transmission.save(specularObj, MATERIAL_LAYER_TRANSMISSION, ctx);
		if (!m_reflection90.isDefault() || !m_reflection90Level.isDefault()) {
			m_reflection90Level.save(specularObj, MATERIAL_LAYER_REFLECTION_90_LEVEL);
			m_reflection90.save(specularObj, MATERIAL_LAYER_REFLECTION_90, ctx);
		}
		m_roughness.save(specularObj, MATERIAL_LAYER_ROUGHNESS, ctx);
		if (!m_anisotropy.isDefault() || !m_anisotropyAngle.isDefault()) {
			m_anisotropy.save(specularObj, MATERIAL_LAYER_ANISOTROPY, ctx);
			m_anisotropyAngle.save(specularObj, MATERIAL_LAYER_ANISOTROPY_ANGLE, ctx);
		}

		if (m_thinFilmInterference.get() || !m_iorFilm.isDefault()) {
			QJsonObject filmObj;
			if (!m_thinFilmInterference.get())
				m_thinFilmInterference.save(filmObj, MATERIAL_LAYER_THIN_FILM_ENABLE);
			m_iorFilm.save(filmObj, ctx);
			m_thickness.save(filmObj, MATERIAL_LAYER_FILM_THICKNESS, ctx);
			m_minThickness.save(filmObj, MATERIAL_LAYER_MIN_FILM_THICKNESS);
			specularObj[MATERIAL_LAYER_THIN_FILM] = filmObj;
		}

		obj[MATERIAL_LAYER_SPECULAR] = specularObj;
	}
}

QByteArray MaterialLayerImpl::save(const ModelSavingContext &ctx) const
{
	QJsonObject layerObj;
	_save(layerObj, ctx);
	return QJsonDocument(layerObj).toJson(ctx.targetFolder.isEmpty() ? QJsonDocument::Compact : QJsonDocument::Indented);
}

// ------------------------------------------------------------------------ //

bool MaterialLayerImpl::compareLayerMaskAndBump(const MaterialLayerImpl & otherLayer) const
{
	return m_mask.isEqual(otherLayer.m_mask) && m_bump.isEqual(otherLayer.m_bump);
}

void MaterialLayerImpl::copyDiffuseLayer(const MaterialLayerImpl & srcLayer)
{
	m_diffuseLayer._set(srcLayer.m_diffuseLayer.get());
	m_diffuseColor.copyFrom(srcLayer.m_diffuseColor);
	m_diffuseOpacity.copyFrom(srcLayer.m_diffuseOpacity);
	m_diffuseTransmission.copyFrom(srcLayer.m_diffuseTransmission);
	updateParams(-1);
}

// ------------------------------------------------------------------------ //

CoreInstance & MaterialLayerImpl::core() const
{
	return m_material.core();
}

void MaterialLayerImpl::pushCommand(QUndoCommand *cmd)
{
	m_material.pushCommand(cmd);
}

void MaterialLayerImpl::fireChanged(eParamId paramId)
{
	// if (paramId == PID_LAYER_IOR) {
	// 	vec3 refl, refl90;
	// 	float refl90Level = 0.f;
	// 	if (calculateReflectionFromMeasuredIOR(refl, refl90, refl90Level, &m_ior)) {
	// 		qDebug() << QString("Reflection from measured IOR:")
	// 				<< " Layer:" << m_name.get()
	// 				<< " reflection: [" << refl.x << "," << refl.y << "," << refl.z << "],\n"
	// 				<< " reflection-90: [" << refl90.x << "," << refl90.y << "," << refl90.z << "],\n"
	// 				<< " reflection-90-level:" << refl90Level;
	// 	}
	// }

	updateParams(paramId != PID_LAYER ? 1 << (paramId - PID_LAYER) : -1);
	if (m_group)
		m_group->fireChanged(paramId);
}

// ------------------------------------------------------------------------ //

void MaterialLayerImpl::collectFileNames(QSet<QString> &res) const
{
	res.insert(TextureImpl::safeGetFileName(m_mask.texture()));
	res.insert(TextureImpl::safeGetFileName(m_bump.texture()));
	res.insert(TextureImpl::safeGetFileName(m_diffuseColor.texture()));
	res.insert(TextureImpl::safeGetFileName(m_diffuseOpacity.texture()));
	res.insert(TextureImpl::safeGetFileName(m_diffuseTransmission.texture()));
	res.insert(TextureImpl::safeGetFileName(m_reflection.texture()));
	res.insert(TextureImpl::safeGetFileName(m_reflection90.texture()));
	res.insert(TextureImpl::safeGetFileName(m_reflection90Level.texture()));
	res.insert(TextureImpl::safeGetFileName(m_transmission.texture()));
	res.insert(TextureImpl::safeGetFileName(m_roughness.texture()));
	res.insert(TextureImpl::safeGetFileName(m_anisotropy.texture()));
	res.insert(TextureImpl::safeGetFileName(m_anisotropyAngle.texture()));
	res.insert(TextureImpl::safeGetFileName(m_thickness.texture()));
	res.insert(TextureImpl::safeGetFileName(m_minThickness.texture()));
	res.insert(TextureImpl::safeGetFileName(m_emissiveColor.texture()));
	res.insert(IndexOfRefractionImpl::safeGetFileName(m_ior));
	res.insert(IndexOfRefractionImpl::safeGetFileName(m_iorFilm));
}

void MaterialLayerImpl::updateFileNames(const QMap<QString, QString>& map)
{
	TextureImpl::updateFileNames(map, m_mask.texture());
	TextureImpl::updateFileNames(map, m_bump.texture());
	TextureImpl::updateFileNames(map, m_diffuseColor.texture());
	TextureImpl::updateFileNames(map, m_diffuseOpacity.texture());
	TextureImpl::updateFileNames(map, m_diffuseTransmission.texture());
	TextureImpl::updateFileNames(map, m_reflection.texture());
	TextureImpl::updateFileNames(map, m_reflection90.texture());
	TextureImpl::updateFileNames(map, m_reflection90Level.texture());
	TextureImpl::updateFileNames(map, m_transmission.texture());
	TextureImpl::updateFileNames(map, m_roughness.texture());
	TextureImpl::updateFileNames(map, m_anisotropy.texture());
	TextureImpl::updateFileNames(map, m_anisotropyAngle.texture());
	TextureImpl::updateFileNames(map, m_thickness.texture());
	TextureImpl::updateFileNames(map, m_minThickness.texture());
	TextureImpl::updateFileNames(map, m_emissiveColor.texture());
	IndexOfRefractionImpl::updateFileNames(map, m_ior);
	IndexOfRefractionImpl::updateFileNames(map, m_iorFilm);
}

// ------------------------------------------------------------------------ //

bool MaterialLayerImpl::calculateReflectionFromMeasuredIOR(vec3 & outReflection, vec3 & outReflection90, float & outReflection90Level, const IndexOfRefractionImpl * ior) const
{
	const auto * targetIOR = ior ? ior : &m_ior;
	if (!targetIOR || targetIOR->iorType() != IORType::MEASURED || !targetIOR->isDynamic())
		return false;

	auto clampColor = [](const vec3 & v) {
		return vec3(std::clamp(v.x, 0.f, 1.f), std::clamp(v.y, 0.f, 1.f), std::clamp(v.z, 0.f, 1.f));
	};

	const vec3 r0 = clampColor(getFresnelReflectance(1.f, nullptr, targetIOR));
	const vec3 r90 = clampColor(getFresnelReflectance(0.f, nullptr, targetIOR));

	outReflection = ColorUtils::linearToSRGB(r0);
	outReflection90 = ColorUtils::linearToSRGB(r90);

	// Fit reflection90Level to best match the spectral Fresnel curve between 0 and 90 degrees
	constexpr int numSamples = 32;
	float targetAngle[numSamples];
	vec3 targetRefl[numSamples];

	for (int i = 0; i < numSamples; ++i) {
		const float cosI = (float)(i + 1) / (float)(numSamples + 1);
		targetAngle[i] = std::acos(cosI) * (2.f * M_1_PIf);
		targetRefl[i] = getFresnelReflectance(cosI, nullptr, targetIOR);
	}

	auto computeError = [&](float level) {
		float totalError = 0.f;
		for (int i = 0; i < numSamples; ++i) {
			float angle = targetAngle[i];
			if (level <= 0.5f)
				angle = std::pow(angle, 0.5f / level);
			else
				angle = 1.f - std::pow(1.f - angle, 0.5f / (1.f - level));

			const vec3 pred = lerp(r0, r90, angle);
			const vec3 diff = pred - targetRefl[i];
			totalError += diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
		}
		return totalError;
	};

	float bestLevel = 0.5f;
	float minError = 1e30f;

	// Coarse search in [0.01, 0.99] with step 0.01
	for (int step = 1; step <= 99; ++step) {
		const float level = step * 0.01f;
		const float err = computeError(level);
		if (err < minError) {
			minError = err;
			bestLevel = level;
		}
	}

	// Refine search around bestLevel (+/- 0.01 with step 0.001)
	const float refineMin = std::max(0.005f, bestLevel - 0.01f);
	const float refineMax = std::min(0.995f, bestLevel + 0.01f);
	for (float level = refineMin; level <= refineMax; level += 0.001f) {
		const float err = computeError(level);
		if (err < minError) {
			minError = err;
			bestLevel = level;
		}
	}

	outReflection90Level = std::clamp(bestLevel, 0.01f, 1.f);
	return true;
}
