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

#include "ScalarParameterImpl.h"

ScalarParameterImpl::ScalarParameterImpl(float value, float min, float max, int precision, eParamId paramId, IParameterOwner & owner, float scale)
	: m_min(min)
	, m_max(max)
	, m_precision(precision)
	, m_scale(scale)
	, m_defaultValue(value)
	, m_owner(owner)
	, m_id(paramId)
	, m_enabled(true)
	, m_visible(true)
{
	m_value = { value, realValue(value) };
}

ScalarParameterImpl::~ScalarParameterImpl()
{
}

vec3 ScalarParameterImpl::realValue(float userValue) const
{
	return vec3(userValue * m_scale);
}

void ScalarParameterImpl::setEnabled(bool b)
{
	if (m_enabled != b) {
		m_enabled = b;
		emit changed();
	}
}

void ScalarParameterImpl::setVisible(bool b)
{
	if (m_visible != b) {
		m_visible = b;
		emit changed();
	}
}

void ScalarParameterImpl::fireChanged(eParamId paramId)
{
	emit changed();
	m_owner.fireChanged(m_id);
}

void ScalarParameterImpl::set(float value)
{
	if (!isfinite(value))
		return;

	value = std::clamp<float>(value, m_min, m_max);
	if (m_value.user != value)
		m_owner.pushCommand(new ParamCommand<ScalarParameterImpl, SValue>(*this, m_owner.core().scene(), m_value, { value, realValue(value) }, m_id));
	else
		emit changed();
}

void ScalarParameterImpl::_set(float value)
{
	if (!isfinite(value))
		return;

	value = std::clamp<float>(value, m_min, m_max);
	if (m_value.user != value) {
		m_value = { value, realValue(value) };
		emit changed();
	}
}

bool ScalarParameterImpl::load(const QDomElement & node, const char * name)
{
	bool res = loadFloatParam(m_value.user, node, name, 1.f / m_scale);
	m_value.user = std::clamp<float>(m_value.user, m_min, m_max);
	m_value.real = realValue(m_value.user);
	return res;
}

void ScalarParameterImpl::save(QJsonObject & obj, const char * name) const
{
	obj[name] = m_value.user * m_scale;
}

bool ScalarParameterImpl::load(const QJsonObject & obj, const char * name)
{
	if (!obj.contains(name))
		return false;

	m_value.user = obj[name].toDouble() * (1.f / m_scale);
	m_value.user = std::clamp<float>(m_value.user, m_min, m_max);
	m_value.real = realValue(m_value.user);
	return true;
}

// ------------------------------------------------------------------------ //

ColorIntensityParameter::ColorIntensityParameter(float value, float min, float max, int precision, eParamId paramId, IParameterOwner & owner)
	: ScalarParameterImpl(value, min, max, precision, paramId, owner, 0.01f)
{
	m_value = { value, realValue(value) };
}

vec3 ColorIntensityParameter::realValue(float userValue) const
{
	return vec3(userValue * m_scale);
}

// ------------------------------------------------------------------------ //

SRGBColorIntensityParameter::SRGBColorIntensityParameter(float value, float min, float max, int precision, eParamId paramId, IParameterOwner & owner)
	: ColorIntensityParameter(value, min, max, precision, paramId, owner)
{
	m_value = { value, realValue(value) };
}

vec3 SRGBColorIntensityParameter::realValue(float userValue) const
{
	return ColorUtils::sRGBToLinear(vec3(userValue * m_scale));
}

// ------------------------------------------------------------------------ //

vec3 RoughnessParameter::realValue(float userValue) const
{
	return vec3(sqr(userValue * m_scale));
}
