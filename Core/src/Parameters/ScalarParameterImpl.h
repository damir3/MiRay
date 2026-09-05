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

class CORE_EXPORT ScalarParameterImpl : public IScalarParameter
{
	Q_OBJECT

protected:
	IParameterOwner & m_owner;
	const eParamId m_id;
	const float m_defaultValue;
	const float m_min;
	const float m_max;
	const int   m_precision;
	const float m_scale;
	struct SValue {
		float user;
		vec3  real;
	} m_value;
	bool m_enabled;
	bool m_visible;

	virtual vec3 realValue(float userValue) const;

public:
	explicit ScalarParameterImpl(float value, float min, float max, int precision, eParamId paramId, IParameterOwner & owner, float scale = 1.f);
	~ScalarParameterImpl();

	float min() const final override { return m_min; }
	float max() const final override { return m_max; }
	int precision() const override { return m_precision; }

	ITexture * texture() override { return nullptr; }
	const ITexture * texture() const override { return nullptr; }

	float get() const final override { return m_value.user; }
	void set(float) final override;
	void _set(float value) final override;

	inline float value() const { return m_value.real.x; }

	bool load(const QDomElement & node, const char * name);
	void save(QJsonObject & obj, const char * name) const;
	bool load(const QJsonObject & obj, const char * name);

	bool isEnabled() const final override { return m_enabled; }
	void setEnabled(bool b);

	bool isVisible() const final override { return m_visible; }
	void setVisible(bool b);

	virtual bool isDefault() const { return m_value.user == m_defaultValue; }

	void fireChanged(eParamId paramId);
};

class ColorIntensityParameter : public ScalarParameterImpl
{
	Q_OBJECT

protected:
	vec3 realValue(float userValue) const override;

public:
	explicit ColorIntensityParameter(float value, float min, float max, int precision, eParamId paramId, IParameterOwner & owner);

	inline const vec3 & rgb() const
	{
		return m_value.real;
	}
};

class SRGBColorIntensityParameter : public ColorIntensityParameter
{
	Q_OBJECT

protected:
	vec3 realValue(float userValue) const override;

public:
	explicit SRGBColorIntensityParameter(float value, float min, float max, int precision, eParamId paramId, IParameterOwner & owner);
};

class RoughnessParameter : public ScalarParameterImpl
{
	Q_OBJECT

protected:
	vec3 realValue(float userValue) const override;

public:
	explicit RoughnessParameter(float value, eParamId paramId, IParameterOwner & owner)
		: ScalarParameterImpl(value, 0.f, 100.f, 1, paramId, owner, 0.01f) {}
};
