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

#include "MaterialLayerImpl.h"

class MaterialImpl;

class MaterialGroupImpl final : public IMaterialGroup, public IParameterOwner
{
	Q_OBJECT

	MaterialImpl &				m_material;
	StringParameterImpl			m_name;
	BooleanParameterImpl		m_enabled;
	TextureScalarParameterImpl	m_mask;
	MaterialLayersList			m_layers;
	std::vector<MaterialLayerImpl *>	m_visibleLayers;
	bool	m_isEmissive;

	void updateEmission();

public:
	MaterialGroupImpl(MaterialImpl & material);
	~MaterialGroupImpl() override;

	MaterialImpl & material() const { return m_material; }

	StringParameterImpl & name() final override { return m_name; }

	BooleanParameterImpl & enabled() final override { return m_enabled; }

	TextureScalarParameterImpl & mask() final override { return m_mask; }

	bool isEmissive() const { return m_isEmissive; }

	void collectFileNames(QSet<QString> &res) const;
	void updateFileNames(const QMap<QString, QString>& map);

	size_t numLayers() const final override { return m_layers.size(); }
	MaterialLayerImpl *layer(size_t i) const final override { return i < m_layers.size() ? m_layers[i].get() : nullptr; }
	const decltype(m_visibleLayers) & layers() const { return m_visibleLayers; }

	MaterialLayerImpl *addLayer(size_t i, const QByteArray & data, const ModelLoadingContext &ctx) final override;
	void removeLayer(size_t i) final override;
	void moveLayer(size_t from, size_t to) final override;

	void _addLayer(MaterialLayerPtr layer, size_t pos);
	MaterialLayerPtr _removeLayer(size_t pos);
	IMaterialLayer * _createLayer(size_t pos) override;

	void _loadParams(const QJsonObject & params, const ModelLoadingContext &ctx);
	bool _load(const QDomElement & node, const ModelLoadingContext &ctx, uint16_t version);
	bool _load(const QJsonObject & obj, const ModelLoadingContext &ctx);
	void _save(QJsonObject & obj, const ModelSavingContext &ctx) const;

	bool load(const QByteArray & data, const ModelLoadingContext &ctx) override;
	QByteArray save(const ModelSavingContext &ctx) const override;

	// IParameterOwner
	CoreInstance & core() const override;
	void pushCommand(QUndoCommand *cmd) override;
	void fireChanged(eParamId paramId) override;

	void updateLayers();
};

using MaterialGroupPtr = std::unique_ptr<MaterialGroupImpl>;
using MaterialGroupsList = std::vector<MaterialGroupPtr>;
