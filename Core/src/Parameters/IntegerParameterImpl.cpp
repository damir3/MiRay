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

#include "IntegerParameterImpl.h"

IntegerParameterImpl::IntegerParameterImpl(int value, int min, int max, eParamId paramId, IParameterOwner & owner)
	: m_min(min)
	, m_max(max)
	, m_value(value)
	, m_owner(owner)
	, m_id(paramId)
	, m_enabled(true)
	, m_visible(true)
{
}

IntegerParameterImpl::~IntegerParameterImpl()
{
}

void IntegerParameterImpl::setEnabled(bool b)
{
	if (m_enabled != b) {
		m_enabled = b;
		emit changed();
	}
}

void IntegerParameterImpl::setVisible(bool b)
{
	if (m_visible != b) {
		m_visible = b;
		emit changed();
	}
}

void IntegerParameterImpl::fireChanged(eParamId paramId)
{
	emit changed();
	m_owner.fireChanged(m_id);
}

void IntegerParameterImpl::setMax(int max)
{
	if (m_max != max) {
		m_max = max;
		m_value = std::min(m_value, m_max);
		emit changed();
	}
}

void IntegerParameterImpl::set(int value)
{
	value = std::clamp<int>(value, m_min, m_max);
	if (m_value != value)
		m_owner.pushCommand(new ParamCommand<IntegerParameterImpl, int>(*this, m_owner.core().scene(), m_value, value, m_id));
	else
		emit changed();
}

void IntegerParameterImpl::_set(int value)
{
	if (m_value != value) {
		m_value = value;
		emit changed();
	}
}

bool IntegerParameterImpl::load(const QDomElement & node, const char * name)
{
	bool res = loadIntParam(m_value, node, name);
	m_value = std::clamp<int>(m_value, m_min, m_max);
	return res;
}

void IntegerParameterImpl::save(QJsonObject & obj, const char * name) const
{
	obj[name] = m_value;
}

bool IntegerParameterImpl::load(const QJsonObject & obj, const char * name)
{
	if (!obj.contains(name))
		return false;

	m_value = obj[name].toInt();
	m_value = std::clamp<int>(m_value, m_min, m_max);
	return true;
}
