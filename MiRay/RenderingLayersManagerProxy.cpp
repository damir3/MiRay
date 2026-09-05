#include "RenderingLayersManagerProxy.h"

#include "../Shared/Interfaces/RenderLayerManager.h"
#include "../Shared/Interfaces/RenderLayer.h"
#include "../Shared/Interfaces/Parameters.h"

RenderingLayersManagerProxy::RenderingLayersManagerProxy(IRenderLayerManager& manager, QObject* parent)
	: QObject(parent)
	, m_manager(manager)
{
	connect(&m_manager, SIGNAL(changed()), this, SLOT(onListChanged()));
	connect(&m_manager, SIGNAL(stateChanged(IRenderLayer*)), this, SLOT(onLayerChanged(IRenderLayer*)));
	updateLayers();
}

RenderingLayersManagerProxy::~RenderingLayersManagerProxy()
{
	disconnect(&m_manager, SIGNAL(changed()), this, SLOT(onListChanged()));
	disconnect(&m_manager, SIGNAL(stateChanged(IRenderLayer*)), this, SLOT(onLayerChanged(IRenderLayer*)));
}

void RenderingLayersManagerProxy::updateLayers()
{
	m_proxies.clear();
	m_layersList.clear();

	for (int i = 0, count = m_manager.count(); i < count; ++i) {
		m_proxies.emplace_back(std::make_unique<StringParamProxy>(""));
		m_proxies.back()->setParam(&m_manager.get(i)->name());
	}

	for (auto& proxy : m_proxies) {
		m_layersList.push_back(QVariant::fromValue(static_cast<QObject*>(proxy.get())));
	}

	emit layersChanged();
}

void RenderingLayersManagerProxy::createLayer()
{
	m_manager.create();
}

void RenderingLayersManagerProxy::removeLayer(int i)
{
	if (i >= 0 && i < (int)m_proxies.size()) {
		m_manager.remove(i);
	}
}

void RenderingLayersManagerProxy::onListChanged()
{
	updateLayers();
}

void RenderingLayersManagerProxy::onLayerChanged(IRenderLayer* layer)
{
}
