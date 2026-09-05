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

#pragma once

class Scene;
#include "RenderLayerImpl.h"
#include "../../Shared/Interfaces/RenderLayerManager.h"

class RenderLayerManager final : public IRenderLayerManager
{
	Q_OBJECT

	Scene &	m_scene;
	std::vector<std::shared_ptr<RenderLayerImpl>>	m_renderLayers;
	std::unique_ptr<RenderLayerImpl>	m_defaultRenderLayer;

	void updateList();

public:
	RenderLayerManager(Scene & scene);

	Scene & scene() const { return m_scene; }

	int count() const override { return (int)m_renderLayers.size(); }
	RenderLayerImpl *get(int i) const override { return i >= 0 && i < (int)m_renderLayers.size() ? m_renderLayers[i].get() : m_defaultRenderLayer.get(); }
	size_t getIndex(IRenderLayer * renderLayer) const override;

	void _add(std::shared_ptr<RenderLayerImpl> renderLayer, size_t pos);
	std::shared_ptr<RenderLayerImpl> _remove(size_t pos);

	RenderLayerImpl *_create();
	RenderLayerImpl *create(size_t position = (size_t)-1) override;
	void remove(size_t index) override;
	void move(size_t from, size_t pos) override;

	RenderLayerImpl *getByName(const QString & name) const;
	QString getValidName(const QString & name) const;

	void fireChanged();
	void fireStateChanged(IRenderLayer *l);
};
