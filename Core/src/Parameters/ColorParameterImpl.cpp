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

#include "ColorParameterImpl.h"

ColorParameterImpl::SColor::SColor(const vec4 & _sRGB)
	: sRGB(_sRGB)
	, linear(ColorUtils::sRGBToLinear(_sRGB))
{}

ColorParameterImpl::ColorParameterImpl(const vec3 &sRGB, eParamId paramId, IParameterOwner & owner)
	: m_color(vec4(sRGB, 1.f))
	, m_defaultValue(sRGB)
	, m_owner(owner)
	, m_id(paramId)
	, m_enabled(true)
	, m_visible(true)
{
}

ColorParameterImpl::~ColorParameterImpl()
{
}

void ColorParameterImpl::setEnabled(bool b)
{
	if (m_enabled != b) {
		m_enabled = b;
		emit changed();
	}
}

void ColorParameterImpl::setVisible(bool b)
{
	if (m_visible != b) {
		m_visible = b;
		emit changed();
	}
}

void ColorParameterImpl::fireChanged(eParamId paramId)
{
	emit changed();
	m_owner.fireChanged(m_id);
}

void ColorParameterImpl::set(const vec3 & sRGB)
{
	const auto sRGBA = vec4(sRGB.x, sRGB.y, sRGB.z, 1.f);
	if (m_color.sRGB != sRGBA)
		m_owner.pushCommand(new ParamCommand<ColorParameterImpl, SColor>(*this, m_owner.core().scene(), m_color, SColor(sRGBA), m_id));
}

void ColorParameterImpl::_set(const vec3 & sRGB)
{
	const auto sRGBA = vec4(sRGB.x, sRGB.y, sRGB.z, 1.f);
	if (m_color.sRGB != sRGBA) {
		m_color = SColor(sRGBA);
		emit changed();
	}
}

bool ColorParameterImpl::load(const QDomElement & node, const char * name)
{
	auto nodeParam = node.firstChildElement(name);
	if (nodeParam.isNull())
		return false;

	vec3 v(m_color.sRGB);
	if (!nodeParam.isNull()) {
		v = vec3FromString(nodeParam.text(), v);
		m_color = SColor(vec4(v, 1.f));
	}

	return true;
}

void ColorParameterImpl::save(QJsonObject & obj, const char * name) const
{
	obj[name] = toJsonArray(vec3(m_color.sRGB));
}

bool ColorParameterImpl::load(const QJsonObject & obj, const char * name)
{
	if (!obj.contains(name))
		return false;

	vec3 v(m_color.sRGB);
	v = getVec3(obj, name, v);
	m_color = SColor(vec4(v, 1.f));
	return true;
}
