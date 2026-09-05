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

class CORE_EXPORT ColorParameterImpl : public IColorParameter
{
	Q_OBJECT

protected:
	IParameterOwner & m_owner;
	const eParamId m_id;
	const vec3 m_defaultValue;
	struct SColor {
		vec4 sRGB;
		vec4 linear;
		SColor(const vec4 & sRGB);
	} m_color;
	bool m_enabled;
	bool m_visible;

public:
	explicit ColorParameterImpl(const vec3 &sRGB, eParamId paramId, IParameterOwner & owner);
	~ColorParameterImpl() override;

	const vec3 & get() const final override { return reinterpret_cast<const vec3 &>(m_color.sRGB); }
	void set(const vec3 &) final override;
	void _set(const vec3 & c) final override;

	ITexture * texture() override { return nullptr; }
	const ITexture * texture() const override { return nullptr; }

	inline const vec3 & sRGB() const // sRGB color
	{
		return reinterpret_cast<const vec3 &>(m_color.sRGB);
	}

	inline const vec3 & rgb() const // linear color
	{
		return reinterpret_cast<const vec3 &>(m_color.linear);
	}

	inline const vec4 & rgba() const // linear color
	{
		return m_color.linear;
	}

	bool load(const QDomElement & node, const char * name);
	void save(QJsonObject & obj, const char * name) const;
	bool load(const QJsonObject & obj, const char * name);

	bool isEnabled() const final override { return m_enabled; }
	void setEnabled(bool b);

	bool isVisible() const final override { return m_visible; }
	void setVisible(bool b);

	virtual bool isDefault() const { return m_color.sRGB.r == m_defaultValue.x && m_color.sRGB.g == m_defaultValue.y && m_color.sRGB.b == m_defaultValue.z; }

	void fireChanged(eParamId paramId);
};
