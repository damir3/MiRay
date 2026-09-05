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

#include "MaterialGroupImpl.h"
#include "MaterialImpl.h"

#include "../../Shared/Interfaces/ModelLoader.h"
#include "../../Shared/Interfaces/SerializationContext.h"

MaterialGroupImpl::MaterialGroupImpl(MaterialImpl & material)
	: m_material(material)
	, m_name("Group", PID_GROUP_NAME, *this, nullptr)
	, m_enabled(true, PID_GROUP_ENABLED, *this)
	, m_mask(100.f, 0.f, 100.f, 1, PID_GROUP_MASK, *this, 0.01f)
	, m_isEmissive(false)
{
	m_layers.emplace_back(std::make_unique<MaterialLayerImpl>(material, this));
	updateLayers();
}

MaterialGroupImpl::~MaterialGroupImpl()
{
}

// ------------------------------------------------------------------------ //

IMaterialLayer * MaterialGroupImpl::_createLayer(size_t pos)
{
	auto layer = std::make_unique<MaterialLayerImpl>(m_material, nullptr);
	auto layerPtr = layer.get();
	_addLayer(std::move(layer), pos);
	return layerPtr;
}

void MaterialGroupImpl::_addLayer(MaterialLayerPtr layer, size_t pos)
{
	assert(pos <= m_layers.size());
	layer->setGroup(this);
	if (pos < m_layers.size())
		m_layers.insert(m_layers.begin() + pos, std::move(layer));
	else
		m_layers.push_back(std::move(layer));
}

MaterialLayerPtr MaterialGroupImpl::_removeLayer(size_t pos)
{
	assert(pos < m_layers.size());
	auto layer = std::move(m_layers[pos]);
	layer->setGroup(nullptr);
	m_layers.erase(m_layers.begin() + pos);
	return layer;
}

// ------------------------------------------------------------------------ //

class MaterialGroupAddLayerCommand : public QUndoCommand
{
	MaterialGroupImpl & m_group;
	MaterialLayerPtr	m_layer;
	size_t	m_position;

	void redo() override
	{
		m_group._addLayer(std::move(m_layer), m_position);
		m_group.fireChanged(PID_GROUP_LAYERS);
	}

	void undo() override
	{
		m_layer = m_group._removeLayer(m_position);
		m_group.fireChanged(PID_GROUP_LAYERS);
	}

public:
	MaterialGroupAddLayerCommand(MaterialGroupImpl & group, MaterialLayerPtr layer, size_t pos)
		: m_group(group)
		, m_layer(std::move(layer))
		, m_position(pos)
	{
	}
};

class MaterialGroupRemoveLayerCommand : public QUndoCommand
{
	MaterialGroupImpl & m_group;
	MaterialLayerPtr	m_layer;
	size_t	m_position;

	void redo() override
	{
		m_layer = m_group._removeLayer(m_position);
		m_group.fireChanged(PID_GROUP_LAYERS);
	}

	void undo() override
	{
		m_group._addLayer(std::move(m_layer), m_position);
		m_group.fireChanged(PID_GROUP_LAYERS);
	}

public:
	MaterialGroupRemoveLayerCommand(MaterialGroupImpl & group, size_t pos)
		: m_group(group)
		, m_layer(nullptr)
		, m_position(pos)
	{
	}
};

class MaterialGroupMoveLayerCommand : public QUndoCommand
{
	MaterialGroupImpl & m_group;
	size_t	m_from, m_to;

	void redo() override
	{
		auto layer = m_group._removeLayer(m_from);
		m_group._addLayer(std::move(layer), m_to);
		m_group.fireChanged(PID_GROUP_LAYERS);
	}

	void undo() override
	{
		auto layer = m_group._removeLayer(m_to);
		m_group._addLayer(std::move(layer), m_from);
		m_group.fireChanged(PID_GROUP_LAYERS);
	}

public:
	MaterialGroupMoveLayerCommand(MaterialGroupImpl & group, size_t from, size_t to)
		: m_group(group)
		, m_from(from)
		, m_to(to)
	{
	}
};

MaterialLayerImpl *MaterialGroupImpl::addLayer(size_t i, const QByteArray & data, const ModelLoadingContext &ctx)
{
	if (i > m_layers.size())
		return nullptr;

	auto layer = std::make_unique<MaterialLayerImpl>(m_material, nullptr);
	if (!m_layers.empty())
		layer->name()._set(QString("Layer %1").arg((int)m_layers.size()));

	if (!data.isNull()) {
		QJsonParseError parseError;
		QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);
		if (parseError.error == QJsonParseError::NoError && !jsonDoc.isNull() && jsonDoc.isObject()) {
			layer->_load(jsonDoc.object(), ctx);
		} else {
			qDebug() << "MaterialGroupImpl::addLayer: Unable to load layer data.";
		}
	}
	auto layerPtr = layer.get();
	m_material.pushCommand(new MaterialGroupAddLayerCommand(*this, std::move(layer), i));
	return layerPtr;
}

void MaterialGroupImpl::removeLayer(size_t i)
{
	if (i >= m_layers.size())
		return;

	m_material.pushCommand(new MaterialGroupRemoveLayerCommand(*this, i));
}

void MaterialGroupImpl::moveLayer(size_t from, size_t to)
{
	if (from == to || from >= m_layers.size() || to >= m_layers.size())
		return;

	m_material.pushCommand(new MaterialGroupMoveLayerCommand(*this, from, to));
}

// ------------------------------------------------------------------------ //

#define MATERIAL_GROUP_NAME					"name"
#define MATERIAL_GROUP_LAYER				"layer"
#define MATERIAL_GROUP_LAYERS				"layers"
#define MATERIAL_GROUP_ENABLE				"enable"
#define MATERIAL_GROUP_MASK					"mask"
#define MATERIAL_GROUP_EMISSION				"emission"
#define MATERIAL_GROUP_EMISSION_INTENSITY	"emission-intensity"

void MaterialGroupImpl::_loadParams(const QJsonObject & params, const ModelLoadingContext &ctx)
{
	assert(!m_layers.empty());
	auto layer = m_layers.front().get();

	if (params.contains(MATERIAL_PARAMETER_EMISSIVE_COLOR)) {
		layer->emissiveColor()._set(ColorUtils::fromString(params[MATERIAL_PARAMETER_EMISSIVE_COLOR].toString(), vec4(0.f, 0.f, 0.f, 1.f)));
		layer->emissiveLayer()._set(!isBlack(layer->emissiveColor().rgb()));
	}

	if (params.contains(MATERIAL_PARAMETER_OPACITY_FACTOR)) {
		layer->mask()._set(params[MATERIAL_PARAMETER_OPACITY_FACTOR].toDouble() * 100.f);
	}

	if (params.contains(MATERIAL_PARAMETER_DIFFUSE_TEXTURE)) {
		layer->diffuseColor().texture()->_set(true, params[MATERIAL_PARAMETER_DIFFUSE_TEXTURE].toString());
	}

	if (params.contains(MATERIAL_PARAMETER_DIFFUSE_COLOR)) {
		layer->diffuseLayer()._set(true);
		layer->diffuseColor()._set(ColorUtils::fromString(params[MATERIAL_PARAMETER_DIFFUSE_COLOR].toString(), vec4(1.f, 1.f, 1.f, 1.f)));
	}

	if (params.contains(MATERIAL_PARAMETER_SPECULAR_COLOR)) {
		auto specularColor = vec3(ColorUtils::fromString(params.value(MATERIAL_PARAMETER_SPECULAR_COLOR).toString("#ffffff"), vec4(1.f, 1.f, 1.f, 1.f)));
		if (!isBlack(specularColor)) {
			layer->reflection()._set(specularColor / maxComponent(specularColor));
			layer->specularLayer()._set(true);
		}
	}

	if (params.contains(MATERIAL_PARAMETER_SPECULAR_TEXTURE)) {
		layer->reflection().texture()->_set(true, params[MATERIAL_PARAMETER_SPECULAR_TEXTURE].toString());
		layer->specularLayer()._set(true);
	}

	bool hasN = params.contains(MATERIAL_PARAMETER_REFRACTION_N);
	bool hasK = params.contains(MATERIAL_PARAMETER_REFRACTION_K);
	if (hasN || hasK) {
		float refractionN = hasN ? params[MATERIAL_PARAMETER_REFRACTION_N].toDouble() : 1.f;
		float refractionK = hasK ? params[MATERIAL_PARAMETER_REFRACTION_K].toDouble() : 0.f;

		if (refractionN <= 1.f)
			hasN = false;

		if (refractionK == 0.f)
			hasK = false;

		if (hasN)
			layer->indexOfRefraction().n()._set(refractionN);

		if (hasK)
			layer->indexOfRefraction().k()._set(refractionK);

		if (hasN && hasK)
			layer->indexOfRefraction().type()._setIndex(1);

		if (hasN || hasK)
			layer->specularLayer()._set(true);
	}

	if (params.contains(MATERIAL_PARAMETER_REFLECTION_BLUR)) {
		float f = params[MATERIAL_PARAMETER_REFLECTION_BLUR].toDouble();
		layer->roughness()._set(f * 100.f);
	}

	if (params.contains(MATERIAL_PARAMETER_REFLECTION_BLUR_MASK)) {
		auto tex = params[MATERIAL_PARAMETER_REFLECTION_BLUR_MASK].toString();
		layer->roughness().texture()->_set(true, tex);
	}

	if (params.contains(MATERIAL_PARAMETER_GLOSSINESS_TEXTURE)) {
		auto tex = params[MATERIAL_PARAMETER_GLOSSINESS_TEXTURE].toString();
		layer->roughness().texture()->_set(true, tex);
	}

	bool testBumpFactor = false;
	if (params.contains(MATERIAL_PARAMETER_BUMP_TEXTURE)) {
		layer->bump().texture()->_set(true, params[MATERIAL_PARAMETER_BUMP_TEXTURE].toString());
		testBumpFactor = true;
	}

	if (params.contains(MATERIAL_PARAMETER_BUMP_NORMAL_MAP)) {
		layer->bump().texture()->_set(true, params[MATERIAL_PARAMETER_BUMP_NORMAL_MAP].toString(), RectF(0.f, 0.f, 1.f, 1.f),
									  ITexture::WRAP_REPEAT, ITexture::WRAP_REPEAT, ITexture::MAPPING_UV0,
									  vec2(1.f, 1.f), vec2(0.f, 0.f), 0.f, false, 0.f, 0.f, 1.f, true);
		testBumpFactor = true;
	}

	if (testBumpFactor) {
		if (params.contains(MATERIAL_PARAMETER_BUMP_FACTOR))
			layer->bump()._set(params[MATERIAL_PARAMETER_BUMP_FACTOR].toDouble() * 0.1f);
		else
			layer->bump()._set(0.1f);
	}

	layer->fireChanged(PID_LAYER);
}

bool MaterialGroupImpl::_load(const QDomElement & node, const ModelLoadingContext &ctx, uint16_t version)
{
	m_visibleLayers.clear();
	m_layers.clear();

	m_name._set(node.attribute(MATERIAL_GROUP_NAME));
	m_enabled.load(node, MATERIAL_GROUP_ENABLE);
	m_mask.load(node, MATERIAL_GROUP_MASK, ctx);

	TextureColorParameterImpl m_emission(vec3(0.f), PID_GROUP_EMISSION, *this);
	ScalarParameterImpl	emissionIntensity(1.f, 0.f, FLT_MAX, 2, PID_GROUP_EMISSION_INTENSITY, *this);
	m_emission.load(node, MATERIAL_GROUP_EMISSION, ctx);
	emissionIntensity.load(node, MATERIAL_GROUP_EMISSION_INTENSITY);
	if (!isBlack(m_emission.rgb() * ColorUtils::sRGBToLinear(emissionIntensity.get()))) {
		auto layer = std::make_unique<MaterialLayerImpl>(m_material, this);
		layer->name()._set("Emissive");
		layer->emissiveLayer()._set(true);
		layer->diffuseLayer()._set(false);
		layer->specularLayer()._set(false);
		layer->emissiveColor().copyFrom(m_emission);
		layer->emissiveIntensity()._set(ColorUtils::sRGBToLinear(emissionIntensity.get()) * 100.f);
		layer->updateParams(-1);
		m_layers.emplace_back(std::move(layer));
	}

	auto layers = node.elementsByTagName(MATERIAL_GROUP_LAYER);
	for (int i = 0; i < layers.size(); i++) {
		auto layer = std::make_unique<MaterialLayerImpl>(m_material, this);
		layer->_load(layers.item(i).toElement(), ctx, version);
		if (!m_layers.empty() && layer->diffuseLayer().get() && !layer->specularLayer().get() && !layer->emissiveLayer().get() &&
			!m_layers.back()->diffuseLayer().get() && m_layers.back()->compareLayerMaskAndBump(*layer)) {
			if (m_layers.back()->name().get() != layer->name().get())
				m_layers.back()->name()._set(m_layers.back()->name().get() + " + " + layer->name().get());

			if (!m_layers.back()->enabled().get()) {
				m_layers.back()->specularLayer()._set(false);
				m_layers.back()->emissiveLayer()._set(false);
			}

			if (!layer->enabled().get())
				layer->diffuseLayer()._set(false);

			m_layers.back()->enabled()._set(true);

			m_layers.back()->copyDiffuseLayer(*layer);
			continue;
		}

		m_layers.emplace_back(std::move(layer));
	}

	updateLayers();

	return true;
}

bool MaterialGroupImpl::_load(const QJsonObject & obj, const ModelLoadingContext &ctx)
{
	m_visibleLayers.clear();
	m_layers.clear();

	m_name._set(obj.value(MATERIAL_GROUP_NAME).toString());
	m_enabled.load(obj, MATERIAL_GROUP_ENABLE);
	m_mask.load(obj, MATERIAL_GROUP_MASK, ctx);

	TextureColorParameterImpl m_emission(vec3(0.f), PID_GROUP_EMISSION, *this);
	ScalarParameterImpl	emissionIntensity(1.f, 0.f, FLT_MAX, 2, PID_GROUP_EMISSION_INTENSITY, *this);
	m_emission.load(obj, MATERIAL_GROUP_EMISSION, ctx);
	emissionIntensity.load(obj, MATERIAL_GROUP_EMISSION_INTENSITY);
	if (!isBlack(m_emission.rgb() * ColorUtils::sRGBToLinear(emissionIntensity.get()))) {
		auto layer = std::make_unique<MaterialLayerImpl>(m_material, this);
		layer->name()._set("Emissive");
		layer->emissiveLayer()._set(true);
		layer->diffuseLayer()._set(false);
		layer->specularLayer()._set(false);
		layer->emissiveColor().copyFrom(m_emission);
		layer->emissiveIntensity()._set(ColorUtils::sRGBToLinear(emissionIntensity.get()) * 100.f);
		layer->updateParams(-1);
		m_layers.emplace_back(std::move(layer));
	}

	if (obj.contains(MATERIAL_GROUP_LAYERS)) {
		QJsonArray layersArr = obj[MATERIAL_GROUP_LAYERS].toArray();
		for (int i = 0; i < layersArr.size(); i++) {
			auto layer = std::make_unique<MaterialLayerImpl>(m_material, this);
			QJsonObject layerObj = layersArr[i].toObject();
			layer->_load(layerObj, ctx);

			if (!m_layers.empty() && layer->diffuseLayer().get() && !layer->specularLayer().get() && !layer->emissiveLayer().get() &&
				!m_layers.back()->diffuseLayer().get() && m_layers.back()->compareLayerMaskAndBump(*layer)) {
				if (m_layers.back()->name().get() != layer->name().get())
					m_layers.back()->name()._set(m_layers.back()->name().get() + " + " + layer->name().get());

				if (!m_layers.back()->enabled().get()) {
					m_layers.back()->specularLayer()._set(false);
					m_layers.back()->emissiveLayer()._set(false);
				}

				if (!layer->enabled().get())
					layer->diffuseLayer()._set(false);

				m_layers.back()->enabled()._set(true);

				m_layers.back()->copyDiffuseLayer(*layer);
				continue;
			}

			m_layers.emplace_back(std::move(layer));
		}
	}

	updateLayers();
	return true;
}

void MaterialGroupImpl::_save(QJsonObject & obj, const ModelSavingContext &ctx) const
{
	obj[MATERIAL_GROUP_NAME] = m_name.get();

	if (!m_enabled.get())
		m_enabled.save(obj, MATERIAL_GROUP_ENABLE);
	if (!m_mask.isDefault())
		m_mask.save(obj, MATERIAL_GROUP_MASK, ctx);

	QJsonArray layersArr;
	for (const auto & layer : m_layers) {
		QJsonObject layerObj;
		layer->_save(layerObj, ctx);
		layersArr.append(layerObj);
	}
	obj[MATERIAL_GROUP_LAYERS] = layersArr;
}


bool MaterialGroupImpl::load(const QByteArray & data, const ModelLoadingContext &ctx)
{
	QJsonParseError parseError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);
	if (parseError.error == QJsonParseError::NoError && !jsonDoc.isNull() && jsonDoc.isObject()) {
		const auto b = _load(jsonDoc.object(), ctx);
		fireChanged(PID_GROUP);
		return b;
	}

	return false;
}

QByteArray MaterialGroupImpl::save(const ModelSavingContext &ctx) const
{
	QJsonObject groupObj;
	_save(groupObj, ctx);
	return QJsonDocument(groupObj).toJson(ctx.targetFolder.isEmpty() ? QJsonDocument::Compact : QJsonDocument::Indented);
}

// ------------------------------------------------------------------------ //

void MaterialGroupImpl::updateLayers()
{
	m_visibleLayers.clear();
	m_visibleLayers.reserve(m_layers.size());

	for (const auto & layer : m_layers) {
		if (layer->enabled().get() && layer->mask().value() > 0.f && (layer->specularLayer().get() || layer->diffuseLayer().get() || layer->emissiveLayer().get()))
			m_visibleLayers.push_back(layer.get());
		else
			layer->setNeighbours(nullptr, nullptr);
	}

	for (size_t i = 0; i < m_visibleLayers.size(); i++) {
		m_visibleLayers[i]->setNeighbours(i > 0 ? m_visibleLayers[i - 1] : nullptr,
										  i + 1 < m_visibleLayers.size() ? m_visibleLayers[i + 1] : nullptr);
	}

	updateEmission();
}

void MaterialGroupImpl::updateEmission()
{
	m_isEmissive = false;
	for (auto layer : m_visibleLayers) {
		if (layer->specularLayer().get())
			break; // ignore emission under the specular layer

		if (layer->isEmissive()) {
			m_isEmissive = true;
			break;
		}
	}
}

CoreInstance & MaterialGroupImpl::core() const
{
	return m_material.core();
}

void MaterialGroupImpl::pushCommand(QUndoCommand *cmd)
{
	m_material.pushCommand(cmd);
}

void MaterialGroupImpl::fireChanged(eParamId paramId)
{
	switch (paramId) {
		case PID_LAYER:
		case PID_LAYER_ENABLED:
		case PID_LAYER_MASK:
		case PID_LAYER_DIFFUSE_LAYER:
		case PID_LAYER_EMISSIVE_LAYER:
		case PID_LAYER_SPECULAR_LAYER:
		case PID_GROUP_LAYERS:
			updateLayers();
			break;
		default:
			updateEmission();
			break;
	}

	//if (paramId == PID_GROUP || paramId == PID_GROUP_LAYERS || paramId == PID_LAYER_ENABLED)
	//	emit changed();

	m_material.fireChanged(paramId);
}

// ------------------------------------------------------------------------ //

void MaterialGroupImpl::collectFileNames(QSet<QString> &res) const
{
	res.insert(TextureImpl::safeGetFileName(m_mask.texture()));

	for (const auto & l : m_layers)
		l->collectFileNames(res);
}

void MaterialGroupImpl::updateFileNames(const QMap<QString, QString>& map)
{
	TextureImpl::updateFileNames(map, m_mask.texture());

	for (const auto & l : m_layers)
		l->updateFileNames(map);
}
