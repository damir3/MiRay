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

#include "EnumParameterImpl.h"

static int getEnumItemsCount(const char * const * items)
{
	int count = 0;
	while (items[count])
		count++;

	return count;
}

EnumParameterImpl::EnumParameterImpl(int index, const char * const * items, eParamId paramId, IParameterOwner & owner)
	: m_items(items)
	, m_count(::getEnumItemsCount(items))
	, m_index(index)
	, m_owner(owner)
	, m_id(paramId)
	, m_enabled(true)
	, m_visible(true)
{
}

EnumParameterImpl::~EnumParameterImpl()
{
}

void EnumParameterImpl::setEnabled(bool b)
{
	if (m_enabled != b) {
		m_enabled = b;
		emit changed();
	}
}

void EnumParameterImpl::setVisible(bool b)
{
	if (m_visible != b) {
		m_visible = b;
		emit changed();
	}
}

void EnumParameterImpl::fireChanged(eParamId paramId)
{
	emit changed();
	m_owner.fireChanged(m_id);
}

void EnumParameterImpl::setIndex(int i)
{
	i = std::clamp<int>(i, 0, m_count - 1);
	if (m_index != i)
		m_owner.pushCommand(new ParamCommand<EnumParameterImpl, int>(*this, m_owner.core().scene(), m_index, i, m_id));
}

void EnumParameterImpl::_setIndex(int i)
{
	i = std::clamp<int>(i, 0, m_count - 1);
	if (m_index != i) {
		m_index = i;
		emit changed();
	}
}

bool EnumParameterImpl::load(const QDomElement & node, const char * name)
{
	bool res = loadIntParam(m_index, node, name);
	m_index = std::clamp<int>(m_index, 0, m_count - 1);
	return res;
}

void EnumParameterImpl::save(QJsonObject & obj, const char * name) const
{
	obj[name] = m_index;
}

bool EnumParameterImpl::load(const QJsonObject & obj, const char * name)
{
	if (!obj.contains(name))
		return false;

	m_index = obj[name].toInt();
	m_index = std::clamp<int>(m_index, 0, m_count - 1);
	return true;
}
