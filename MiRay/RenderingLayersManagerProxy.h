#pragma once

#include "ParamProxy.h"

class IRenderLayer;
class IRenderLayerManager;

class RenderingLayersManagerProxy : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QVariantList layers READ layers NOTIFY layersChanged)

	IRenderLayerManager& m_manager;
	QVariantList m_layersList;
	std::vector<std::unique_ptr<StringParamProxy>> m_proxies;

	void updateLayers();

public:
	RenderingLayersManagerProxy(IRenderLayerManager& manager, QObject* parent = nullptr);
	~RenderingLayersManagerProxy() override;

	QVariantList layers() const { return m_layersList; }

	Q_INVOKABLE void createLayer();
	Q_INVOKABLE void removeLayer(int uiIndex);

private slots:
	void onListChanged();
	void onLayerChanged(IRenderLayer*);

signals:
	void layersChanged();
};
