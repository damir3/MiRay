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

#include "StringParameterImpl.h"

StringParameterImpl::StringParameterImpl(const QString & value, eParamId paramId, IParameterOwner & owner, const IStringParameterValidator * validator)
	: m_owner(owner)
	, m_id(paramId)
	, m_validator(validator)
	, m_value(value)
	, m_enabled(true)
	, m_visible(true)
{
}

StringParameterImpl::~StringParameterImpl()
{
}

void StringParameterImpl::setEnabled(bool b)
{
	if (m_enabled != b) {
		m_enabled = b;
		emit changed();
	}
}

void StringParameterImpl::setVisible(bool b)
{
	if (m_visible != b) {
		m_visible = b;
		emit changed();
	}
}

void StringParameterImpl::fireChanged(eParamId paramId)
{
	emit changed();
	m_owner.fireChanged(m_id);
}

void StringParameterImpl::set(const QString & str)
{
	auto validated = m_validator ? m_validator->validate(this, str) : str;
	if (m_value.compare(validated))
		m_owner.pushCommand(new ParamCommand<StringParameterImpl, QString>(*this, m_owner.core().scene(), m_value, validated, m_id));
	else
		emit changed();
}

void StringParameterImpl::_set(const QString & str)
{
	auto validated = m_validator ? m_validator->validate(this, str) : str;
	if (m_value.compare(validated)) {
		m_value = validated;
		emit changed();
	}
}

bool StringParameterImpl::load(const QDomElement & node, const char * name)
{
	return loadStringParam(m_value, node, name);
}

void StringParameterImpl::save(QJsonObject & obj, const char * name) const
{
	obj[name] = m_value;
}

bool StringParameterImpl::load(const QJsonObject & obj, const char * name)
{
	if (!obj.contains(name))
		return false;

	m_value = obj[name].toString();
	return true;
}
