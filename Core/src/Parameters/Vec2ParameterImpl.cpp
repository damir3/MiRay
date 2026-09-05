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

#include "Vec2ParameterImpl.h"

Vec2ParameterImpl::Vec2ParameterImpl(const vec2 &value, int precision, eParamId paramId, IParameterOwner & owner)
	: m_precision(precision)
	, m_value(value)
	, m_owner(owner)
	, m_id(paramId)
	, m_enabled(true)
	, m_visible(true)
{
}

Vec2ParameterImpl::~Vec2ParameterImpl()
{
}

void Vec2ParameterImpl::setEnabled(bool b)
{
	if (m_enabled != b) {
		m_enabled = b;
		emit changed();
	}
}

void Vec2ParameterImpl::setVisible(bool b)
{
	if (m_visible != b) {
		m_visible = b;
		emit changed();
	}
}

void Vec2ParameterImpl::fireChanged(eParamId paramId)
{
	emit changed();
	m_owner.fireChanged(m_id);
}

void Vec2ParameterImpl::set(const vec2 & v)
{
	if (m_value != v && isfinite(v.x) && isfinite(v.y))
		m_owner.pushCommand(new ParamCommand<Vec2ParameterImpl, vec2>(*this, m_owner.core().scene(), m_value, v, m_id));
}

void Vec2ParameterImpl::_set(const vec2 & v)
{
	if (m_value != v && isfinite(v.x) && isfinite(v.y)) {
		m_value = v;
		emit changed();
	}
}

bool Vec2ParameterImpl::load(const QDomElement & node, const char * name)
{
	loadVec2Param(m_value, node, name);
	return true;
}

void Vec2ParameterImpl::save(QJsonObject & obj, const char * name) const
{
	obj[name] = toJsonArray(m_value);
}

bool Vec2ParameterImpl::load(const QJsonObject & obj, const char * name)
{
	if (!obj.contains(name))
		return false;

	m_value = getVec2(obj, name, m_value);
	return true;
}
