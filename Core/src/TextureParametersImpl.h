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

#pragma once

#include "TextureImpl.h"

class TextureColorParameterImpl final : public ColorParameterImpl, public IParameterOwner
{
	Q_OBJECT

	TextureImpl m_texture;

public:
	explicit TextureColorParameterImpl(const vec3 &color, eParamId paramId, IParameterOwner & owner, TextureImpl::eType texType = TextureImpl::TYPE_COLOR);
	~TextureColorParameterImpl() override;

	TextureImpl * texture() final override { return &m_texture; }
	const TextureImpl * texture() const final override { return &m_texture; }

	vec3 getColor(const vec2 & tc) const
	{
		return m_texture.valid() ? m_texture.getColor(tc) * rgb() : rgb();
	}

	vec3 getColor(const Ray & ray) const
	{
		return m_texture.valid() ? m_texture.getColor(ray) * rgb() : rgb();
	}

	vec4 getColor4(const vec2 & tc) const
	{
		return m_texture.valid() ? m_texture.getColor4(tc) * rgba() : rgba();
	}

	vec4 getColor4(const Ray & ray) const
	{
		return m_texture.valid() ? m_texture.getColor4(ray) * rgba() : rgba();
	}

	float getOpacity(const Ray & ray) const
	{
		return m_texture.valid() ? m_texture.getOpacity(ray) : 1.f;
	}

	bool isDefault() const final override { return ColorParameterImpl::isDefault() && m_texture.isDefault(); }

	bool load(const QDomElement & node, const char * name, const ModelLoadingContext &ctx);
	bool load(const QJsonObject & obj, const char * name, const ModelLoadingContext &ctx);
	void save(QJsonObject & obj, const char * name, const ModelSavingContext &ctx) const;

	QJsonObject getState() const;
	void setState(const QJsonObject & state);

	void copyFrom(const TextureColorParameterImpl & src);

	struct TransitionState {
		vec4 color;
		TextureImpl::TransitionState texture;
	};

	void getTransitionState(TransitionState & state) const;
	void setTransitionState(const TransitionState & state);

	CoreInstance & core() const override { return m_owner.core(); }
	void pushCommand(QUndoCommand *cmd) override { m_owner.pushCommand(cmd); }
	void fireChanged(eParamId paramId) override;
};

class TextureScalarParameterImpl : public ScalarParameterImpl, public IParameterOwner
{
	Q_OBJECT

	TextureImpl	m_texture;

public:
	explicit TextureScalarParameterImpl(float value, float min, float max, int precision, eParamId paramId,
										IParameterOwner & owner, float scale = 1.f, TextureImpl::eType texType = TextureImpl::TYPE_SCALAR);
	~TextureScalarParameterImpl() override;

	TextureImpl * texture() final override { return &m_texture; }
	const TextureImpl * texture() const final override { return &m_texture; }

	float getScalar(const Ray & ray) const
	{
		return m_texture.valid() ? m_texture.getScalar(ray) * m_value.real.x : m_value.real.x;
	}

	bool isDefault() const final override { return ScalarParameterImpl::isDefault() && m_texture.isDefault(); }

	bool load(const QDomElement & node, const char * name, const ModelLoadingContext &ctx);
	bool load(const QJsonObject & obj, const char * name, const ModelLoadingContext &ctx);
	void save(QJsonObject & obj, const char * name, const ModelSavingContext &ctx) const;

	bool isEqual(const TextureScalarParameterImpl & other) const;
	void copyFrom(const TextureScalarParameterImpl & src);

	CoreInstance & core() const override { return m_owner.core(); }
	void pushCommand(QUndoCommand *cmd) override { m_owner.pushCommand(cmd); }
	void fireChanged(eParamId paramId) override;
};
