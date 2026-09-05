/*
	Copyright (C) 2018-2020 Damir Sagidullin

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

#include "RenderLayerManager.h"

// ------------------------------------------------------------------------ //

RenderLayerManager::RenderLayerManager(Scene & scene)
	: m_scene(scene)
	, m_defaultRenderLayer(new RenderLayerImpl(scene, "Default"))
{
}

// ------------------------------------------------------------------------ //

size_t RenderLayerManager::getIndex(IRenderLayer * renderLayer) const
{
	size_t i = 0;
	while (i < m_renderLayers.size() && m_renderLayers[i].get() != renderLayer) i++;
	return i;
}

// ------------------------------------------------------------------------ //

void RenderLayerManager::updateList()
{
	int i = 1;
	for (auto & renderLayer : m_renderLayers)
		renderLayer->_setIndex(i++);
}

// ------------------------------------------------------------------------ //

RenderLayerImpl *RenderLayerManager::getByName(const QString & name) const
{
	for (auto & renderLayer : m_renderLayers) {
		if (renderLayer->name().get() == name)
			return renderLayer.get();
	}

	return nullptr;
}

QString RenderLayerManager::getValidName(const QString & name) const
{
	if (getByName(name) == nullptr)
		return name;

	int index = 0;
	while (true) {
		QString newName = name + QString().asprintf(" %d", ++index);
		if (getByName(newName) == nullptr)
			return newName;
	}

	return name;
}

// ------------------------------------------------------------------------ //

void RenderLayerManager::_add(std::shared_ptr<RenderLayerImpl> renderLayer, size_t pos)
{
	assert(pos <= m_renderLayers.size());
	if (pos < m_renderLayers.size())
		m_renderLayers.insert(m_renderLayers.begin() + pos, renderLayer);
	else
		m_renderLayers.push_back(renderLayer);

	updateList();
}

std::shared_ptr<RenderLayerImpl> RenderLayerManager::_remove(size_t pos)
{
	assert(pos < m_renderLayers.size());
	auto renderLayer = m_renderLayers[pos];
	m_renderLayers.erase(m_renderLayers.begin() + pos);
	updateList();
	renderLayer->_setIndex(0);
	return renderLayer;
}

// ------------------------------------------------------------------------ //

class AddRenderLayerCommand : public QUndoCommand
{
	RenderLayerManager &	m_manager;
	std::shared_ptr<RenderLayerImpl>	m_renderLayer;
	size_t					m_position;

	void redo() override
	{
		m_manager.scene().lock(SceneInternalModification_RenderLayers);
		m_manager._add(m_renderLayer, m_position);
		m_renderLayer.reset();
		m_manager.fireChanged();
		m_manager.scene().unlock(SceneInternalModification_RenderLayers);
	}

	void undo() override
	{
		m_manager.scene().lock(SceneInternalModification_RenderLayers);
		m_renderLayer = m_manager._remove(m_position);
		m_manager.fireChanged();
		m_manager.scene().unlock(SceneInternalModification_RenderLayers);
	}

public:
	AddRenderLayerCommand(RenderLayerManager & manager, std::shared_ptr<RenderLayerImpl> renderLayer, size_t pos)
		: m_manager(manager)
		, m_renderLayer(renderLayer)
		, m_position(pos)
	{
	}
};

class RemoveRenderLayerCommand final : public QUndoCommand
{
	RenderLayerManager &	m_manager;
	std::shared_ptr<RenderLayerImpl>	m_renderLayer;
	size_t					m_position;
	std::vector<MeshNode *>	m_meshNodes;

	void redo() override
	{
		m_manager.scene().lock(SceneInternalModification_RenderLayers);
		m_renderLayer = m_manager._remove(m_position);
		for (auto * meshNode : m_meshNodes)
			meshNode->renderLayer()._setIndex(0);
		m_manager.fireChanged();
		m_manager.scene().unlock(SceneInternalModification_RenderLayers);
	}

	void undo() override
	{
		m_manager.scene().lock(SceneInternalModification_RenderLayers);
		m_manager._add(m_renderLayer, m_position);
		for (auto * meshNode : m_meshNodes)
			meshNode->renderLayer()._setValue(m_renderLayer.get());
		m_renderLayer.reset();
		m_manager.fireChanged();
		m_manager.scene().unlock(SceneInternalModification_RenderLayers);
	}

public:
	RemoveRenderLayerCommand(RenderLayerManager & manager, size_t pos)
		: m_manager(manager)
		, m_renderLayer(nullptr)
		, m_position(pos)
	{
		manager.scene().root().findMeshNodesByRenderLayer(m_meshNodes, m_manager.get((int)pos));
	}
};

class MoveRenderLayerCommand : public QUndoCommand
{
	RenderLayerManager &	m_manager;
	size_t					m_from, m_to;

	void redo() override
	{
		m_manager.scene().lock(SceneInternalModification_RenderLayers);
		m_manager._add(m_manager._remove(m_from), m_to);
		m_manager.fireChanged();
		m_manager.scene().unlock(SceneInternalModification_RenderLayers);
	}

	void undo() override
	{
		m_manager.scene().lock(SceneInternalModification_RenderLayers);
		m_manager._add(m_manager._remove(m_to), m_from);
		m_manager.fireChanged();
		m_manager.scene().unlock(SceneInternalModification_RenderLayers);
	}

public:
	MoveRenderLayerCommand(RenderLayerManager & manager, size_t from, size_t to)
		: m_manager(manager)
		, m_from(from)
		, m_to(to)
	{
	}
};

RenderLayerImpl *RenderLayerManager::_create()
{
	auto renderLayer = std::make_shared<RenderLayerImpl>(m_scene, getValidName(QString().asprintf("Layer %d", (int)m_renderLayers.size() + 1)));
	m_renderLayers.emplace_back(renderLayer);
	updateList();
	return renderLayer.get();
}

RenderLayerImpl *RenderLayerManager::create(size_t position)
{
	position = std::min(position, m_renderLayers.size());
	auto renderLayer = std::make_shared<RenderLayerImpl>(m_scene, getValidName(QString().asprintf("Layer %d", (int)position + 1)));
	m_scene.pushCommand(new AddRenderLayerCommand(*this, renderLayer, position));
	return renderLayer.get();
}

void RenderLayerManager::remove(size_t index)
{
	if (index < m_renderLayers.size())
		m_scene.pushCommand(new RemoveRenderLayerCommand(*this, index));
}

void RenderLayerManager::move(size_t from, size_t to)
{
	if (from == to || from >= m_renderLayers.size() || to >= m_renderLayers.size())
		return;

	m_scene.pushCommand(new MoveRenderLayerCommand(*this, from, to));
}

// ------------------------------------------------------------------------ //

void RenderLayerManager::fireChanged()
{
	emit changed();
}

void RenderLayerManager::fireStateChanged(IRenderLayer *l)
{
	emit stateChanged(l);
}
