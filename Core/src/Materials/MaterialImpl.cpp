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

#include "MaterialImpl.h"
#include "MaterialGroupImpl.h"
#include "MaterialManager.h"

#include "../../Shared/Interfaces/ModelLoader.h"
#include "../../Shared/Interfaces/SerializationContext.h"

static const char * const MEDIUM_TYPES[] = { "None", "Manual", "Measured", "Opaque", nullptr };

// ------------------------------------------------------------------------ //

MaterialImpl::MaterialImpl(MaterialManager & owner, const QString & name)
	: m_owner(owner)
	, m_guid(QUuid::createUuid())
	, m_name(owner.getValidName(name), PID_MATERIAL_NAME, *this, this)
	, m_bump(0.f, 0.f, 100.f, 3, PID_MATERIAL_BUMP, *this, 1.f, TextureImpl::TYPE_BUMP_MAP) // cm
	, m_medium(MEDIUM_TYPE_NONE, MEDIUM_TYPES, PID_MATERIAL_MEDIUM_TYPE, *this)
	, m_ior(PID_MATERIAL_IOR, 1.5f, *this)
	, m_absorptionColor(vec3(1.f), PID_MATERIAL_ABSORPTION_COLOR, *this)
	, m_absorptionAttenuation(0.f, 0.f, FLT_MAX, 3, PID_MATERIAL_ABSORPTION_ATTENUATION, *this) // cm
	, m_emissionColor(vec3(0.f), PID_MATERIAL_EMISSION_COLOR, *this)
	, m_emissionScale(0.f, 0.f, FLT_MAX, 3, PID_MATERIAL_EMISSION_SCALE, *this) // cm
	, m_subsurfaceScattering(false, PID_MATERIAL_SUBSURFACE_SCATTERING, *this)
	, m_scatteringColor(vec3(0.5f), PID_MATERIAL_SCATTERING_COLOR, *this)
	, m_scatteringScale(1.f, 0.f, 1000.f, 3, PID_MATERIAL_SCATTERING_SCALE, *this)
	, m_scatteringAsymmetry(0.f, -1.f, 1.f, 3, PID_MATERIAL_SCATTERING_ASYMMETRY, *this)
	, m_priority(0, 0, INT_MAX, PID_MATERIAL_PRIORITY, *this)
	, m_doubleSided(true, PID_MATERIAL_DOUBLE_SIDED, *this)
	, m_uniqueColor(owner.generateUniqueColor())
{
	m_groups.emplace_back(std::make_unique<MaterialGroupImpl>(*this));
	updateParams(PID_MATERIAL_MEDIUM_TYPE);
}

MaterialImpl::~MaterialImpl()
{
//	qDebug() << "delete material" << m_name.get() << this;
}

// ------------------------------------------------------------------------ //

#define MATERIAL						"material"
#define MATERIAL_GROUP					"group"
#define MATERIAL_GROUPS					"groups"
#define MATERIAL_BUMP					"bump"
#define MATERIAL_MEDIUM					"medium"
#define MATERIAL_ABSORPTION				"absorption"
#define MATERIAL_ABSORPTION_COLOR		"color"
#define MATERIAL_ABSORPTION_ATTENUATION	"attenuation"
#define MATERIAL_EMISSION				"emission"
#define MATERIAL_EMISSION_COLOR			"color"
#define MATERIAL_EMISSION_SCALE			"scale"
#define MATERIAL_SUBSURFACE_SCATTERING	"subsurface-scattering"
#define MATERIAL_SCATTERING_ENABLE		"enable"
#define MATERIAL_SCATTERING_COLOR		"color"
#define MATERIAL_SCATTERING_SCALE		"scale"
#define MATERIAL_SCATTERING_ASYMMETRY	"asymmetry"
#define MATERIAL_NAME					"name"
#define MATERIAL_GUID					"guid"
#define MATERIAL_VERSION				"version"
#define MATERIAL_THUMBNAIL				"thumbnail"
#define MATERIAL_PRIORITY				"priority"
#define MATERIAL_DOUBLE_SIDED			"double-sided"

bool MaterialImpl::load(const QByteArray & data, const ModelLoadingContext &ctx)
{
	QJsonParseError parseError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);
	if (parseError.error == QJsonParseError::NoError && !jsonDoc.isNull() && jsonDoc.isObject()) {
		return _load(jsonDoc.object(), ctx);
	}

	QDomDocument doc;
	if (!doc.setContent(data))
		return false;

	return _load(doc.documentElement(), ctx);
}

QByteArray MaterialImpl::save(const ModelSavingContext &ctx) const
{
	QJsonObject obj;
	_save(obj, ctx);
	QJsonDocument doc(obj);
	return doc.toJson(QJsonDocument::Indented);
}

bool MaterialImpl::_loadParams(const QJsonObject & params, const ModelLoadingContext &ctx)
{
	// TODO: don't we need to reset the material here, just in case?
	if (params.contains(MATERIAL_GROUP) || params.contains(MATERIAL_NAME)) {
		return _load(params, ctx);
	}

	m_groups.front()->_loadParams(params, ctx);
	return true;
}

bool MaterialImpl::_load(const QJsonObject & obj, const ModelLoadingContext &ctx)
{
	m_name._set(m_owner.getValidName(obj.value(MATERIAL_NAME).toString()));
	if (obj.contains(MATERIAL_GUID))
		m_guid = QUuid(obj.value(MATERIAL_GUID).toString());

	m_bump.load(obj, MATERIAL_BUMP, ctx);

	m_ior.load(obj, ctx);
	if (!m_medium.load(obj, MATERIAL_MEDIUM))
		m_medium._setIndex(m_ior.type().getIndex() == 2 ? MEDIUM_TYPE_MEASURED : MEDIUM_TYPE_MANUAL);

	if (obj.contains(MATERIAL_ABSORPTION)) {
		QJsonObject absorptionObj = obj[MATERIAL_ABSORPTION].toObject();
		m_absorptionColor.load(absorptionObj, MATERIAL_ABSORPTION_COLOR);
		m_absorptionAttenuation.load(absorptionObj, MATERIAL_ABSORPTION_ATTENUATION);
	}

	if (obj.contains(MATERIAL_EMISSION)) {
		QJsonObject emissionObj = obj[MATERIAL_EMISSION].toObject();
		m_emissionColor.load(emissionObj, MATERIAL_EMISSION_COLOR);
		m_emissionScale.load(emissionObj, MATERIAL_EMISSION_SCALE);
	}

	if (obj.contains(MATERIAL_SUBSURFACE_SCATTERING)) {
		QJsonObject sssObj = obj[MATERIAL_SUBSURFACE_SCATTERING].toObject();
		m_subsurfaceScattering.load(sssObj, MATERIAL_SCATTERING_ENABLE);
		m_scatteringColor.load(sssObj, MATERIAL_SCATTERING_COLOR);
		m_scatteringScale.load(sssObj, MATERIAL_SCATTERING_SCALE);
		m_scatteringAsymmetry.load(sssObj, MATERIAL_SCATTERING_ASYMMETRY);
	}

	m_priority.load(obj, MATERIAL_PRIORITY);
	m_doubleSided.load(obj, MATERIAL_DOUBLE_SIDED);

	m_groups.clear();
	if (obj.contains(MATERIAL_GROUPS)) {
		QJsonArray groupsArr = obj[MATERIAL_GROUPS].toArray();
		for (int i = 0; i < groupsArr.size(); i++) {
			auto group = std::make_unique<MaterialGroupImpl>(*this);
			group->_load(groupsArr[i].toObject(), ctx);
			m_groups.emplace_back(std::move(group));
		}
	}

	updateParams(PID_MATERIAL_MEDIUM_TYPE);

	m_validPreview = false;
	m_preview = QImage();

	if (obj.contains(MATERIAL_THUMBNAIL)) {
		auto data = QByteArray::fromBase64(obj.value(MATERIAL_THUMBNAIL).toString().toLocal8Bit());
		try {
			auto imageManager = m_owner.core().appContext().imageManager();
			auto image = imageManager->loadImage(data, QString(), false, eImageColorSpace::sRGB);
			if (image) {
				m_preview = image->toQImage(false);
				m_validPreview = true;
			}
		} catch (const std::exception &) {
		}
	}

	return true;
}

void MaterialImpl::_save(QJsonObject & obj, const ModelSavingContext &ctx) const
{
	static const auto version = QString().asprintf("%d.%d", VER_MAJOR, VER_MINOR);
	obj[MATERIAL_NAME] = m_name.get();
	obj[MATERIAL_GUID] = m_guid.toString();
	obj[MATERIAL_VERSION] = version;

	if (!m_bump.isDefault())
		m_bump.save(obj, MATERIAL_BUMP, ctx);

	m_medium.save(obj, MATERIAL_MEDIUM);

	m_ior.save(obj, ctx);

	if (!isWhite(m_absorptionColor.rgb()) || m_absorptionAttenuation.value() > 0.f) {
		QJsonObject absorptionObj;
		m_absorptionColor.save(absorptionObj, MATERIAL_ABSORPTION_COLOR);
		m_absorptionAttenuation.save(absorptionObj, MATERIAL_ABSORPTION_ATTENUATION);
		obj[MATERIAL_ABSORPTION] = absorptionObj;
	}

	if (!isBlack(m_emissionColor.rgb()) || m_emissionScale.value() > 0.f) {
		QJsonObject emissionObj;
		m_emissionColor.save(emissionObj, MATERIAL_EMISSION_COLOR);
		m_emissionScale.save(emissionObj, MATERIAL_EMISSION_SCALE);
		obj[MATERIAL_EMISSION] = emissionObj;
	}

	if (m_subsurfaceScattering.get()) {
		QJsonObject sssObj;
		m_subsurfaceScattering.save(sssObj, MATERIAL_SCATTERING_ENABLE);
		m_scatteringColor.save(sssObj, MATERIAL_SCATTERING_COLOR);
		m_scatteringScale.save(sssObj, MATERIAL_SCATTERING_SCALE);
		m_scatteringAsymmetry.save(sssObj, MATERIAL_SCATTERING_ASYMMETRY);
		obj[MATERIAL_SUBSURFACE_SCATTERING] = sssObj;
	}

	if (m_priority.get() != 0)
		m_priority.save(obj, MATERIAL_PRIORITY);

	if (!m_doubleSided.get())
		m_doubleSided.save(obj, MATERIAL_DOUBLE_SIDED);

	QJsonArray groupsArr;
	for (const auto & group : m_groups) {
		QJsonObject groupObj;
		group->_save(groupObj, ctx);
		groupsArr.append(groupObj);
	}
	obj[MATERIAL_GROUPS] = groupsArr;

	if (!m_preview.isNull() && m_validPreview) {
		try {
			auto imageManager = m_owner.core().appContext().imageManager();
			auto image = imageManager->createImage(m_preview, eImageFormat::RGBA, eImageDataType::Byte, eImageColorSpace::sRGB);
			auto data = imageManager->saveImage(image, "jpg", 0.9f);
			if (!data.isEmpty()) {
				obj[MATERIAL_THUMBNAIL] = QString::fromLocal8Bit(data.toBase64());
			}
		} catch (const std::exception &) {
		}
	}
}

bool MaterialImpl::_load(const QDomElement & node, const ModelLoadingContext &ctx)
{
	if (node.tagName() != MATERIAL || !node.hasAttribute(MATERIAL_NAME))
		return false;

	m_name._set(m_owner.getValidName(node.attribute(MATERIAL_NAME)));
	m_guid = node.attribute(MATERIAL_GUID);
	const auto strVersion = node.attribute(MATERIAL_VERSION).split('.');
	const uint16_t version = strVersion.size() >= 2 ? (strVersion[0].toUInt() << 8) | (strVersion[1].toUInt()) :
		(ctx.fileVersion == 0xFFFF ? 0 : ctx.fileVersion);

	m_bump.load(node, MATERIAL_BUMP, ctx);

	m_ior.load(node, ctx);
	if (!m_medium.load(node, MATERIAL_MEDIUM))
		m_medium._setIndex(m_ior.type().getIndex() == 2 ? MEDIUM_TYPE_MEASURED : MEDIUM_TYPE_MANUAL);

	auto absorptionNode = node.firstChildElement(MATERIAL_ABSORPTION);
	if (!absorptionNode.isNull()) {
		m_absorptionColor.load(absorptionNode, MATERIAL_ABSORPTION_COLOR);
		m_absorptionAttenuation.load(absorptionNode, MATERIAL_ABSORPTION_ATTENUATION);
	}

	auto emissionNode = node.firstChildElement(MATERIAL_EMISSION);
	if (!emissionNode.isNull()) {
		m_emissionColor.load(emissionNode, MATERIAL_EMISSION_COLOR);
		m_emissionScale.load(emissionNode, MATERIAL_EMISSION_SCALE);
	}

	auto sssNode = node.firstChildElement(MATERIAL_SUBSURFACE_SCATTERING);
	if (!sssNode.isNull()) {
		m_subsurfaceScattering.load(sssNode, MATERIAL_SCATTERING_ENABLE);
		m_scatteringColor.load(sssNode, MATERIAL_SCATTERING_COLOR);
		m_scatteringScale.load(sssNode, MATERIAL_SCATTERING_SCALE);
		m_scatteringAsymmetry.load(sssNode, MATERIAL_SCATTERING_ASYMMETRY);
	}

	m_priority.load(node, MATERIAL_PRIORITY);
	m_doubleSided.load(node, MATERIAL_DOUBLE_SIDED);

	m_groups.clear();
	auto groups = node.elementsByTagName(MATERIAL_GROUP);
	for (int i = 0; i < groups.size(); i++) {
		auto group = std::make_unique<MaterialGroupImpl>(*this);
		group->_load(groups.item(i).toElement(), ctx, version);
		m_groups.emplace_back(std::move(group));
	}

	updateParams(PID_MATERIAL_MEDIUM_TYPE);

	m_validPreview = false;
	m_preview = QImage();

	auto thumbnailNode = node.firstChildElement(MATERIAL_THUMBNAIL);
	if (!thumbnailNode.isNull()) {
		auto data = QByteArray::fromBase64(thumbnailNode.text().toLocal8Bit());

		try {
			auto imageManager = m_owner.core().appContext().imageManager();
			auto image = imageManager->loadImage(data, QString(), false, eImageColorSpace::sRGB);
			if (image) {
				m_preview = image->toQImage(false);
				m_validPreview = true;
			}
		} catch (const std::exception &) {
		}
	}

	return true;
}

// ------------------------------------------------------------------------ //

QString MaterialImpl::validate(IStringParameter *property, QString val) const
{
	assert(property == &m_name);

	if (!m_owner.isValidName(val))
		return property->get();

	return val;
}

// ------------------------------------------------------------------------ //

IMaterialGroup *MaterialImpl::_createGroup(size_t pos)
{
	auto group = std::make_unique<MaterialGroupImpl>(*this);
	auto groupPtr = group.get();
	_addGroup(std::move(group), pos);
	return groupPtr;
}

void MaterialImpl::_addGroup(MaterialGroupPtr group, size_t pos)
{
	assert(pos <= m_groups.size());
	if (pos < m_groups.size())
		m_groups.insert(m_groups.begin() + pos, std::move(group));
	else
		m_groups.push_back(std::move(group));
}

MaterialGroupPtr MaterialImpl::_removeGroup(size_t pos)
{
	assert(pos < m_groups.size());
	auto group = std::move(m_groups[pos]);
	m_groups.erase(m_groups.begin() + pos);
	return group;
}

// ------------------------------------------------------------------------ //

class MaterialAddGroupCommand : public QUndoCommand
{
	MaterialImpl & m_material;
	MaterialGroupPtr m_group;
	size_t	m_position;

	void redo() override
	{
		m_material._addGroup(std::move(m_group), m_position);
		m_material.fireChanged(PID_MATERIAL_GROUPS);
	}

	void undo() override
	{
		m_group = m_material._removeGroup(m_position);
		m_material.fireChanged(PID_MATERIAL_GROUPS);
	}

public:
	MaterialAddGroupCommand(MaterialImpl & material, MaterialGroupPtr group, size_t pos)
		: m_material(material)
		, m_group(std::move(group))
		, m_position(pos)
	{
	}
};

class MaterialRemoveGroupCommand : public QUndoCommand
{
	MaterialImpl & m_material;
	MaterialGroupPtr m_group;
	size_t	m_position;

	void redo() override
	{
		m_group = m_material._removeGroup(m_position);
		m_material.fireChanged(PID_MATERIAL_GROUPS);
	}

	void undo() override
	{
		m_material._addGroup(std::move(m_group), m_position);
		m_material.fireChanged(PID_MATERIAL_GROUPS);
	}

public:
	MaterialRemoveGroupCommand(MaterialImpl & material, size_t pos)
		: m_material(material)
		, m_group(nullptr)
		, m_position(pos)
	{
	}
};

class MaterialMoveGroupCommand : public QUndoCommand
{
	MaterialImpl & m_material;
	size_t	m_from, m_to;

	void redo() override
	{
		auto group = m_material._removeGroup(m_from);
		m_material._addGroup(std::move(group), m_to);
		m_material.fireChanged(PID_MATERIAL_GROUPS);
	}

	void undo() override
	{
		auto group = m_material._removeGroup(m_to);
		m_material._addGroup(std::move(group), m_from);
		m_material.fireChanged(PID_MATERIAL_GROUPS);
	}

public:
	MaterialMoveGroupCommand(MaterialImpl & material, size_t from, size_t to)
		: m_material(material)
		, m_from(from)
		, m_to(to)
	{
	}
};

MaterialGroupImpl *MaterialImpl::addGroup(size_t i)
{
	if (i > m_groups.size())
		return nullptr;

	auto group = std::make_unique<MaterialGroupImpl>(*this);
	if (!m_groups.empty())
		group->name()._set(QString("Group %1").arg((int)m_groups.size()));

	auto groupPtr = group.get();
	pushCommand(new MaterialAddGroupCommand(*this, std::move(group), i));
	return groupPtr;
}

void MaterialImpl::removeGroup(size_t i)
{
	if (i >= m_groups.size())
		return;

	pushCommand(new MaterialRemoveGroupCommand(*this, i));
}

void MaterialImpl::moveGroup(size_t from, size_t to)
{
	if (from == to || from >= m_groups.size() || to >= m_groups.size())
		return;

	pushCommand(new MaterialMoveGroupCommand(*this, from, to));
}

// ------------------------------------------------------------------------ //

class MaterialMoveLayerCommand : public QUndoCommand
{
	MaterialGroupImpl & m_groupFrom;
	MaterialGroupImpl & m_groupTo;
	size_t m_layerFrom;
	size_t m_layerTo;

	void redo() override
	{
		auto layer = m_groupFrom._removeLayer(m_layerFrom);
		m_groupFrom.fireChanged(PID_GROUP_LAYERS);

		m_groupTo._addLayer(std::move(layer), m_layerTo);
		m_groupTo.fireChanged(PID_GROUP_LAYERS);
	}

	void undo() override
	{
		auto layer = m_groupTo._removeLayer(m_layerTo);
		m_groupTo.fireChanged(PID_GROUP_LAYERS);

		m_groupFrom._addLayer(std::move(layer), m_layerFrom);
		m_groupFrom.fireChanged(PID_GROUP_LAYERS);
	}

public:
	MaterialMoveLayerCommand(MaterialGroupImpl & groupFrom, size_t layerFrom, MaterialGroupImpl & groupTo, size_t layerTo)
		: m_groupFrom(groupFrom)
		, m_layerFrom(layerFrom)
		, m_groupTo(groupTo)
		, m_layerTo(layerTo)
	{
	}
};

void MaterialImpl::moveLayer(size_t groupFrom, size_t layerFrom, size_t groupTo, size_t layerTo)
{
	if (groupFrom >= m_groups.size() || groupTo >= m_groups.size())
		return;

	if (groupFrom == groupTo && layerFrom == layerTo)
		return;

	auto g1 = m_groups[groupFrom].get();
	auto g2 = m_groups[groupTo].get();
	if (layerFrom >= g1->numLayers() || layerTo > g2->numLayers())
		return;

	pushCommand(new MaterialMoveLayerCommand(*g1, layerFrom, *g2, layerTo));
}

// ------------------------------------------------------------------------ //

void MaterialImpl::pushCommand(QUndoCommand *cmd)
{
	m_owner.scene().pushCommand(cmd);
}

// ------------------------------------------------------------------------ //

void MaterialImpl::updateParams(eParamId paramId)
{
	updateMediumCoefs(paramId);
	updatePreviewColor();
}

void MaterialImpl::updateMediumCoefs(eParamId paramId)
{
	const auto mediumType = (eMediumType)m_medium.getIndex();
	const bool hasMedium = mediumType == MEDIUM_TYPE_MANUAL || mediumType == MEDIUM_TYPE_MEASURED;
	bool updateAbsorption = false;
	bool updateEmission = false;
	bool updateScattering = false;
	switch (paramId) {
		case PID_MATERIAL_MEDIUM_TYPE: {
			updateAbsorption = updateEmission = updateScattering = true;

			m_ior.type().setEnabled(hasMedium);
			m_ior.type().setVisible(hasMedium);
			m_subsurfaceScattering.setEnabled(hasMedium);
			m_subsurfaceScattering.setVisible(hasMedium);
			m_ior.type()._setIndex(mediumType == MEDIUM_TYPE_MEASURED ? 2 : 0);
			m_ior.fireChanged(PID_IOR_TYPE);
			break;
		}
		case PID_MATERIAL_IOR:
		case PID_MATERIAL_ABSORPTION_COLOR:
		case PID_MATERIAL_ABSORPTION_ATTENUATION:
			updateAbsorption = true;
			break;

		case PID_MATERIAL_EMISSION_COLOR:
		case PID_MATERIAL_EMISSION_SCALE:
			updateEmission = true;
			break;

		case PID_MATERIAL_SUBSURFACE_SCATTERING:
		case PID_MATERIAL_SCATTERING_COLOR:
		case PID_MATERIAL_SCATTERING_SCALE:
		case PID_MATERIAL_SCATTERING_ASYMMETRY:
			updateScattering = true;
			break;

		default:
			return;
	}

	if (updateAbsorption) {
		auto enabled = mediumType == MEDIUM_TYPE_MANUAL;
		m_absorptionColor.setEnabled(enabled);
		m_absorptionColor.setVisible(enabled);
		m_absorptionAttenuation.setEnabled(enabled);
		m_absorptionAttenuation.setVisible(enabled);

		auto absorption = mediumType == MEDIUM_TYPE_MEASURED ? m_ior.absorption() : m_absorptionColor.rgb();
		auto attenuation = mediumType == MEDIUM_TYPE_MEASURED ? m_ior.attenuation() : m_absorptionAttenuation.get();
		m_hasAbsorption = hasMedium && attenuation > 0.f && !isWhite(absorption);
		if (m_hasAbsorption) {
			m_absorptionCoef.x = std::log(std::max(absorption.x, 1e-9f));
			m_absorptionCoef.y = std::log(std::max(absorption.y, 1e-9f));
			m_absorptionCoef.z = std::log(std::max(absorption.z, 1e-9f));
			m_absorptionCoef /= attenuation;
		} else
			m_absorptionCoef = vec3(0.f);
	}

	if (updateEmission) {
		m_emissionCoef = hasMedium ? m_emissionColor.rgb() * m_emissionScale.get() : vec3(0.f);
	}

	if (updateScattering) {
		auto enabled = hasMedium && m_subsurfaceScattering.get();
		m_scatteringColor.setEnabled(enabled);
		m_scatteringColor.setVisible(enabled);
		m_scatteringScale.setEnabled(enabled);
		m_scatteringScale.setVisible(enabled);
		m_scatteringAsymmetry.setEnabled(enabled);
		m_scatteringAsymmetry.setVisible(enabled);

		m_hasScattering = enabled && m_scatteringScale.value() > 0.f && m_scatteringAsymmetry.value() < 1.f;
		if (m_hasScattering) {
			m_scatteringCoef = m_scatteringColor.rgb() * m_scatteringScale.value();
			//m_scatteringCoef.x = 1.f / m_scatteringCoef.x;
			//m_scatteringCoef.y = 1.f / m_scatteringCoef.y;
			//m_scatteringCoef.z = 1.f / m_scatteringCoef.z;
		} else
			m_scatteringCoef = vec3(0.f);
	}

	m_attenuationCoef = -m_absorptionCoef + m_scatteringCoef;

	m_minAttenuationCoef = m_attenuationCoef.x;
	m_minAttenuationCoefIndex = 0;
	if (m_attenuationCoef.y > 0 && (m_minAttenuationCoef == 0 || m_attenuationCoef.y < m_minAttenuationCoef))
		m_minAttenuationCoef = m_attenuationCoef.y, m_minAttenuationCoefIndex = 1;
	if (m_attenuationCoef.z > 0 && (m_minAttenuationCoef == 0 || m_attenuationCoef.z < m_minAttenuationCoef))
		m_minAttenuationCoef = m_attenuationCoef.z, m_minAttenuationCoefIndex = 2;

	m_hasAttenuation = m_attenuationCoef.x > 0.f || m_attenuationCoef.y > 0.f || m_attenuationCoef.z > 0.f;

	m_maxBeamLength = m_hasAttenuation ? (-std::log(1e-9f) / maxComponent(m_attenuationCoef)) : std::numeric_limits<float>::infinity();

	m_meanFreePath = m_hasAttenuation ? 1.f / maxComponent(m_attenuationCoef) : std::numeric_limits<float>::infinity();
}

void MaterialImpl::updatePreviewColor()
{
	const auto mediumType = (eMediumType)m_medium.getIndex();
	m_previewColor = vec3(0.f);
	m_previewTexture = nullptr;
	const TextureImpl * previewSpecularTexture = nullptr;
	m_previewEmissiveColor = vec3(0.f);
	m_previewEmissiveTexture = nullptr;
	float groupOpacity = 1.f;
	for (const auto & group : m_groups) {
		if (!group->enabled().get()) continue;
		const auto groupMask = group->mask().value();

		vec3 groupTransmission(groupMask * groupOpacity);
		for (MaterialLayerImpl *layer : group->layers()) {
			auto layerMask = layer->mask().value();
			vec3 layerTransmission(groupTransmission * layerMask);
			if (layer->specularLayer().get()) {
				if (!previewSpecularTexture && !isBlack(layerTransmission) && layer->reflection().texture()->valid())
					previewSpecularTexture = layer->reflection().texture();

				auto reflectance = getFresnelReflectance(0.85f, nullptr, &layer->indexOfRefraction(), 0);
				//auto reflectCoeff = Luminance(reflectance);
				m_previewColor += layerTransmission * layer->reflection().rgb() * reflectance;

				layerTransmission *= (1.f - reflectance);
				if (layer->indexOfRefraction().iorType() == IORType::MEASURED && layer->indexOfRefraction().attenuation() == 0.f)
					layerTransmission = vec3(0.f); // opaque
				else if (layer->transmission().isEnabled())
					layerTransmission *= layer->transmission().rgb();
			}

			if (layer->emissiveLayer().get()) {
				if (!m_previewEmissiveTexture && !isBlack(layerTransmission) && layer->emissiveColor().texture()->valid())
					m_previewEmissiveTexture = layer->emissiveColor().texture();

				m_previewEmissiveColor += layer->emissiveColor().rgb() * layer->emissiveIntensity().rgb() * layerTransmission;
			}

			if (layer->diffuseLayer().get()) {
				if (!m_previewTexture && !isBlack(layerTransmission) && layer->diffuseColor().texture()->valid())
					m_previewTexture = layer->diffuseColor().texture();

				m_previewColor += layer->diffuseColor().rgb() * layerTransmission;
				layerTransmission *= layer->diffuseTransmission().rgb() * (1.f - layer->diffuseOpacity().value());
			}

			groupTransmission = groupTransmission * (1.f - layerMask) + layerTransmission;
		}

		if (!m_previewTexture)
			m_previewTexture = previewSpecularTexture;

		if (mediumType != MEDIUM_TYPE_OPAQUE) {
			if (m_hasAttenuation) {
				const float baseColor = 0.75f;
				vec3 attenuation(1.f);
				float dist = 100.f;
				float maxAttenuation;
				for (int i = 0; i < 15; i++) {
					attenuation = evalAttenuation(dist);
					maxAttenuation = std::max(std::max(attenuation.x, attenuation.y), attenuation.z);
					if (i < 10) {
						if (maxAttenuation > baseColor)
							i = 10;
						else
							dist *= 0.5f;
					} else {
						if (maxAttenuation < baseColor) break;
						dist *= 1.15f;
					}
				}
				attenuation /= maxAttenuation;

				groupTransmission *= attenuation;
			}

			m_previewColor += groupTransmission;
		}
		groupOpacity *= (1.f - groupMask);
	}

	m_previewEmission = !isBlack(m_previewEmissiveColor);
}

vec3 MaterialImpl::getPreviewColor(const Ray & ray) const
{
	vec3 color;
	if (m_previewTexture) {
		auto textureColor = m_previewTexture->getColor4(ray);
		color = glm::mix(m_previewColor, vec3(textureColor) * m_previewColor, textureColor.a);
	} else
		color = m_previewColor;

	static const vec3 lightDir = glm::normalize(vec3(1.f));
	color *= glm::dot(ray.normal, lightDir) * 0.4f + 0.6f;

	if (m_previewEmission) {
		if (m_previewEmissiveTexture) {
			auto textureColor = m_previewEmissiveTexture->getColor4(ray);
			color += glm::mix(m_previewEmissiveColor, vec3(textureColor) * m_previewEmissiveColor, textureColor.a);
		} else
			color += m_previewEmissiveColor;
	}

	return color;
}

void MaterialImpl::update()
{
	updateParams(PID_MATERIAL_MEDIUM_TYPE);

	for (const auto & group : m_groups) {
		group->updateLayers();
		for (size_t li = 0; li < group->numLayers(); li++)
			group->layer(li)->updateParams(-1);
	}
}

inline float evalAttenuationInOneDim(float aAttenuationCoef, float aDistanceAlongRay)
{
	return std::exp(-aAttenuationCoef * aDistanceAlongRay);
}

vec3 MaterialImpl::evalAttenuation(const float dist) const
{
	assert(dist >= 0.f);
	return vec3(evalAttenuationInOneDim(m_attenuationCoef.x, dist),
				evalAttenuationInOneDim(m_attenuationCoef.y, dist),
				evalAttenuationInOneDim(m_attenuationCoef.z, dist));
}

float MaterialImpl::sampleRay(const float aDistToBoundary,
							  const float aRandom,
							  float * oPdf,
							  const uint32_t aRaySamplingFlags,
							  float * oRevPdf) const
{
	assert(aDistToBoundary >= 0);
	assert(aRandom >= 0.f && aRandom < 1.f);

	if (m_hasScattering) { // we can sample along the ray
		float s = -std::log(aRandom) / m_minAttenuationCoef;

		if (s < aDistToBoundary) { // sample is before the boundary intersection
			float att = evalAttenuationInOneDim(m_minAttenuationCoef, s);

			if (oPdf)
				*oPdf = m_minAttenuationCoef * att;

			if (oRevPdf)
				*oRevPdf = (aRaySamplingFlags & kOriginInMedium) ? *oPdf : att;

			return s;
		} else { // sample is behind the boundary intersection
			float att = evalAttenuationInOneDim(m_minAttenuationCoef, aDistToBoundary);

			if (oPdf)
				*oPdf = att;

			if (oRevPdf)
				*oRevPdf = (aRaySamplingFlags & kOriginInMedium) ? m_minAttenuationCoef * att : att;

			return aDistToBoundary;
		}
	} else { // we cannot sample along the ray
		if (oPdf) *oPdf = 1.0f;
		if (oRevPdf) *oRevPdf = 1.0f;
		return aDistToBoundary;
	}
}

float MaterialImpl::raySamplePdf(const float aDistMin,
								 const float aDistMax,
								 const uint32_t aRaySamplingFlags,
								 float * oRevPdf) const
{
	assert(aDistMin >= 0.f);
	assert(aDistMax >= aDistMin);

	float oPdf = 1.f;
	if (oRevPdf) *oRevPdf = 1.f;

	if (m_hasScattering) { // we can sample along the ray
		float att = std::max(evalAttenuationInOneDim(m_minAttenuationCoef, aDistMax - aDistMin), 1e-35f);
		float minatt = m_minAttenuationCoef * att;

		oPdf = (aRaySamplingFlags & kEndInMedium) ? minatt : att;

		if (oRevPdf)
			*oRevPdf = (aRaySamplingFlags & kOriginInMedium) ? minatt : att;
	}

	return oPdf;
}

// ------------------------------------------------------------------------ //

CoreInstance & MaterialImpl::core() const
{
	return m_owner.core();
}

void MaterialImpl::fireChanged(eParamId paramId)
{
	updateParams(paramId);
	m_validPreview = false;
	if (paramId == PID_MATERIAL || paramId == PID_MATERIAL_GROUPS || paramId == PID_MATERIAL_NAME || paramId == PID_LAYER_ENABLED || paramId == PID_GROUP_ENABLED || paramId == PID_GROUP_LAYERS || paramId == PID_LAYER_NAME || paramId == PID_GROUP_NAME)
		emit changed();
	m_owner.fireMaterialChanged(this, paramId == PID_MATERIAL_GROUPS || paramId == PID_GROUP_LAYERS);
}

// ------------------------------------------------------------------------ //

bool MaterialImpl::isPreviewValid() const
{
	return m_validPreview;
}

QImage MaterialImpl::getPreview() const
{
	return m_preview;
}

void MaterialImpl::setPreview(QImage image)
{
	m_preview = image;
	m_validPreview = true;
}

bool MaterialImpl::getMaterialInformation(const QByteArray &data, IApplicationContext::MaterialInformation &info)
{
	QDomDocument doc;
	if (!doc.setContent(data))
		return false;

	auto root = doc.documentElement();
	if (root.tagName() != MATERIAL || !root.hasAttribute(MATERIAL_NAME))
		return false;

	info.name = root.attribute(MATERIAL_NAME);

	if (root.hasAttribute(MATERIAL_GUID))
		info.guid = root.attribute(MATERIAL_GUID);

	auto thumbnailNode = root.firstChildElement(MATERIAL_THUMBNAIL);
	if (!thumbnailNode.isNull()) {
		auto mtl_data = QByteArray::fromBase64(thumbnailNode.text().toLocal8Bit());
		auto imageManager = qobject_cast<IApplicationContext *>(qApp)->imageManager();
		auto image = imageManager->loadImage(mtl_data, QString(), false, eImageColorSpace::sRGB);
		if (image)
			info.thumbnail = image->toQImage(false);
	}

	return true;
}

// ------------------------------------------------------------------------ //

void MaterialImpl::collectFileNames(QSet<QString> &res) const
{
	res.insert(TextureImpl::safeGetFileName(m_bump.texture()));
	res.insert(TextureImpl::safeGetFileName(m_absorptionColor.texture()));
	res.insert(TextureImpl::safeGetFileName(m_absorptionAttenuation.texture()));
	res.insert(TextureImpl::safeGetFileName(m_emissionColor.texture()));
	res.insert(TextureImpl::safeGetFileName(m_emissionScale.texture()));
	res.insert(TextureImpl::safeGetFileName(m_scatteringColor.texture()));
	res.insert(TextureImpl::safeGetFileName(m_scatteringScale.texture()));
	res.insert(IndexOfRefractionImpl::safeGetFileName(m_ior));

	for (const auto & g : m_groups)
		g->collectFileNames(res);
}

void MaterialImpl::updateFileNames(const QMap<QString, QString>& map)
{
	TextureImpl::updateFileNames(map, m_bump.texture());
	TextureImpl::updateFileNames(map, m_absorptionColor.texture());
	TextureImpl::updateFileNames(map, m_absorptionAttenuation.texture());
	TextureImpl::updateFileNames(map, m_emissionColor.texture());
	TextureImpl::updateFileNames(map, m_emissionScale.texture());
	TextureImpl::updateFileNames(map, m_scatteringColor.texture());
	TextureImpl::updateFileNames(map, m_scatteringScale.texture());
	IndexOfRefractionImpl::updateFileNames(map, m_ior);

	for (const auto & g : m_groups)
		g->updateFileNames(map);
}
