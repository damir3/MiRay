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

#include "RenderLayerImpl.h"

RenderLayerImpl::RenderLayerImpl(Scene & scene, const QString & name)
	: m_scene(scene)
	, m_name(name, PID_RENDER_LAYER_NAME, *this, this)
	, m_index(0)
{
}

// ------------------------------------------------------------------------ //

QString RenderLayerImpl::validate(IStringParameter *property, QString val) const
{
	assert(property == &m_name);

	if (m_scene.renderLayerManager().getByName(val) == nullptr)
		return val;

	return property->get();
}

// ------------------------------------------------------------------------ //

CoreInstance & RenderLayerImpl::core() const
{
	return m_scene.core();
}

void RenderLayerImpl::pushCommand(QUndoCommand *cmd)
{
	m_scene.pushCommand(cmd);
}

void RenderLayerImpl::fireChanged(eParamId paramId)
{
	m_scene.renderLayerManager().fireStateChanged(this);
}

// ------------------------------------------------------------------------ //

RenderLayerEnumParamImpl::RenderLayerEnumParamImpl(RenderLayerManager &manager, eParamId id, IParameterOwner & owner)
	: m_manager(manager)
	, m_owner(owner)
	, m_value(manager.get(-1))
	, m_id(id)
	, m_visible(true)
{
	moveToThread(QApplication::instance()->thread());
	connect(&m_manager, SIGNAL(changed()), this, SLOT(onListChanged()));
	connect(&m_manager, SIGNAL(stateChanged(IRenderLayer *)), this, SLOT(onLayerChanged(IRenderLayer *)));
}

RenderLayerEnumParamImpl::~RenderLayerEnumParamImpl()
{
	disconnect(&m_manager, SIGNAL(changed()), this, SLOT(onListChanged()));
	disconnect(&m_manager, SIGNAL(stateChanged(IRenderLayer *)), this, SLOT(onLayerChanged(IRenderLayer *)));
}

void RenderLayerEnumParamImpl::onListChanged()
{
	emit changed();
}

void RenderLayerEnumParamImpl::onLayerChanged(IRenderLayer * l)
{
	emit changed();
}

int RenderLayerEnumParamImpl::count() const
{
	return (int)m_manager.count() + 1;
}

QString RenderLayerEnumParamImpl::getTitle(int i) const
{
	return m_manager.get(i - 1)->name().get();
}

int RenderLayerEnumParamImpl::getIndex() const
{
	return m_value->getIndex();
}

void RenderLayerEnumParamImpl::setIndex(int i)
{
	auto value = m_manager.get(i - 1);
	m_owner.pushCommand(new ParamCommand<RenderLayerEnumParamImpl, RenderLayerImpl *>(*this, m_owner.core().scene(), m_value, value, m_id));
}

void RenderLayerEnumParamImpl::_setIndex(int i)
{
	_setValue(m_manager.get(i - 1));
}

void RenderLayerEnumParamImpl::_setValue(RenderLayerImpl * value)
{
	m_value = value;
	emit changed();
}

void RenderLayerEnumParamImpl::setVisible(bool b)
{
	if (m_visible != b) {
		m_visible = b;
		emit changed();
	}
}

void RenderLayerEnumParamImpl::fireChanged(eParamId paramId)
{
	emit changed();
	m_owner.fireChanged(m_id);
};
