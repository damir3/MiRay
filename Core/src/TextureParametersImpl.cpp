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

#include "TextureParametersImpl.h"

static const char * PARAM_TEXTURE = "texture";
static const char * PARAM_VALUE = "value";

// ------------------------------------------------------------------------ //

TextureColorParameterImpl::TextureColorParameterImpl(const vec3 &color, eParamId paramId, IParameterOwner & owner, TextureImpl::eType texType)
	: ColorParameterImpl(color, paramId, owner)
	, m_texture(*this, paramId, texType)
{
}

TextureColorParameterImpl::~TextureColorParameterImpl()
{
}

bool TextureColorParameterImpl::load(const QDomElement & node, const char * name, const ModelLoadingContext &ctx)
{
	auto nodeParam = node.firstChildElement(name);
	if (nodeParam.isNull())
		return false;

	vec3 v(m_color.sRGB);
	auto nodeTexture = nodeParam.firstChildElement(PARAM_TEXTURE);
	if (!nodeTexture.isNull()) {
		loadVec3Param(v, nodeParam, PARAM_VALUE);
		m_color = vec4(v, 1.f);

		return m_texture._load(nodeTexture, ctx);
	}

	if (!nodeParam.isNull()) {
		v = vec3FromString(nodeParam.text(), v);
		m_color = vec4(v, 1.f);
	}

	return true;
}

void TextureColorParameterImpl::save(QJsonObject & obj, const char * name, const ModelSavingContext &ctx) const
{
	if (!m_texture.isDefault()) {
		QJsonObject paramObj;
		paramObj[PARAM_VALUE] = toJsonArray(vec3(m_color.sRGB));
		QJsonObject texObj;
		m_texture._save(texObj, ctx);
		paramObj[PARAM_TEXTURE] = texObj;
		obj[name] = paramObj;
		return;
	}

	obj[name] = toJsonArray(vec3(m_color.sRGB));
}

bool TextureColorParameterImpl::load(const QJsonObject & obj, const char * name, const ModelLoadingContext &ctx)
{
	if (!obj.contains(name))
		return false;

	auto val = obj[name];
	if (val.isObject()) {
		QJsonObject paramObj = val.toObject();
		vec3 v(m_color.sRGB);
		v = getVec3(paramObj, PARAM_VALUE, v);
		m_color = vec4(v, 1.f);

		if (paramObj.contains(PARAM_TEXTURE)) {
			return m_texture._load(paramObj[PARAM_TEXTURE].toObject(), ctx);
		}
	} else {
		vec3 v(m_color.sRGB);
		v = getVec3(obj, name, v);
		m_color = vec4(v, 1.f);
	}

	return true;
}

QJsonObject TextureColorParameterImpl::getState() const
{
	return QJsonObject {
		{ "image", m_texture.getState() },
		{ "color", toJsonArray(vec3(m_color.sRGB)) }
	};
}

void TextureColorParameterImpl::setState(const QJsonObject & state)
{
	if (state.isEmpty()) return;

	m_texture.setState(state["image"].toObject());
	vec3 color(m_color.sRGB);
	color = getVec3(state, "color", color);
	m_color = vec4(color, 1.f);
}

void TextureColorParameterImpl::copyFrom(const TextureColorParameterImpl & src)
{
	m_color = src.m_color;
	m_texture.copyFrom(src.m_texture);
}

void TextureColorParameterImpl::getTransitionState(TransitionState & state) const
{
	state.color = m_color.sRGB;
	m_texture.getTransitionState(state.texture);
}

void TextureColorParameterImpl::setTransitionState(const TransitionState & state)
{
	m_color = state.color;
	m_texture.setTransitionState(state.texture);
}

void TextureColorParameterImpl::fireChanged(eParamId paramId)
{
	emit changed();
	m_owner.fireChanged(m_id);
}

// ------------------------------------------------------------------------ //

TextureScalarParameterImpl::TextureScalarParameterImpl(float value, float min, float max, int precision,
													   eParamId paramId, IParameterOwner & owner, float scale, TextureImpl::eType texType)
	: ScalarParameterImpl(value, min, max, precision, paramId, owner, scale)
	, m_texture(*this, paramId, texType)
{
}

TextureScalarParameterImpl::~TextureScalarParameterImpl()
{
}

bool TextureScalarParameterImpl::load(const QDomElement & node, const char * name, const ModelLoadingContext &ctx)
{
	auto nodeParam = node.firstChildElement(name);
	if (nodeParam.isNull())
		return false;

	auto nodeTexture = nodeParam.firstChildElement(PARAM_TEXTURE);
	if (!nodeTexture.isNull()) {
		loadFloatParam(m_value.user, nodeParam, PARAM_VALUE, 1.f / m_scale);
		m_value.user = std::clamp<float>(m_value.user, m_min, m_max);
		m_value.real = realValue(m_value.user);

		return m_texture._load(nodeTexture, ctx);
	}

	if (!nodeParam.isNull()) {
		bool ok;
		float res = nodeParam.text().toFloat(&ok);
		if (ok) {
			m_value.user = std::clamp<float>(res / m_scale, m_min, m_max);
			m_value.real = realValue(m_value.user);
		}
	}

	return true;
}

void TextureScalarParameterImpl::save(QJsonObject & obj, const char * name, const ModelSavingContext &ctx) const
{
	if (!m_texture.isDefault()) {
		QJsonObject paramObj;
		paramObj[PARAM_VALUE] = m_value.user * m_scale;
		QJsonObject texObj;
		m_texture._save(texObj, ctx);
		paramObj[PARAM_TEXTURE] = texObj;
		obj[name] = paramObj;
		return;
	}

	obj[name] = m_value.user * m_scale;
}

bool TextureScalarParameterImpl::load(const QJsonObject & obj, const char * name, const ModelLoadingContext &ctx)
{
	if (!obj.contains(name))
		return false;

	auto val = obj[name];
	if (val.isObject()) {
		QJsonObject paramObj = val.toObject();
		if (paramObj.contains(PARAM_VALUE)) {
			m_value.user = paramObj[PARAM_VALUE].toDouble() * (1.f / m_scale);
			m_value.user = std::clamp<float>(m_value.user, m_min, m_max);
			m_value.real = realValue(m_value.user);
		}
		if (paramObj.contains(PARAM_TEXTURE)) {
			return m_texture._load(paramObj[PARAM_TEXTURE].toObject(), ctx);
		}
	} else {
		m_value.user = val.toDouble() * (1.f / m_scale);
		m_value.user = std::clamp<float>(m_value.user, m_min, m_max);
		m_value.real = realValue(m_value.user);
	}

	return true;
}


bool TextureScalarParameterImpl::isEqual(const TextureScalarParameterImpl & other) const
{
	return m_value.user == other.m_value.user && m_texture.isEqual(other.m_texture);
}

void TextureScalarParameterImpl::copyFrom(const TextureScalarParameterImpl & src)
{
	m_value = src.m_value;
	m_texture.copyFrom(src.m_texture);
}

void TextureScalarParameterImpl::fireChanged(eParamId paramId)
{
	emit changed();
	m_owner.fireChanged(m_id);
}
