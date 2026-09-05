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

#include "../ParametersImpl.h"

#define DEFAULT_WAVELENGTH	550.f

class Random;

class IndexOfRefractionImpl final : public IIndexOfRefraction, public IParameterOwner
{
public:
	struct SpectrumColor {
		float waveLength;
		vec3  xyz;
	};

protected:
	IParameterOwner	&		m_owner;
	const eParamId			m_id;
	EnumParameterImpl		m_type;
	ScalarParameterImpl		m_n;
	ScalarParameterImpl		m_k;
	FileNameParameterImpl	m_fileName;
	std::vector<vec3>		m_iors;

	vec3  m_absorption;
	float m_attenuation;

	static std::vector<SpectrumColor> g_spectrumColors;

	void updateIOR();

public:
	IndexOfRefractionImpl(eParamId paramId, float n, IParameterOwner & owner);

	static void initSpectrumColors();
	static const decltype(g_spectrumColors) & spectrumColors() { return g_spectrumColors; }

	static QString safeGetFileName(const IIndexOfRefraction &ior);
	static void updateFileNames(const QMap<QString, QString> &map, IIndexOfRefraction &ior);

	EnumParameterImpl & type() override { return m_type; }
	ScalarParameterImpl & n() override { return m_n; }
	ScalarParameterImpl & k() override { return m_k; }
	const FileNameParameterImpl & fileName() const { return m_fileName; }
	FileNameParameterImpl & fileName() override { return m_fileName; }

	IORType iorType() const { return (IORType)m_type.getIndex(); }
	bool equals(const IndexOfRefractionImpl * ior) const;

	std::complex<float>  getIOR(float nm = 550.f) const;
	const vec3 & absorption() const { return m_absorption; }
	float attenuation() const { return m_attenuation; }
	bool isDynamic() const { return !m_iors.empty(); }
	bool isDefault() const;

	void setEnabled(bool enabled);
	void setVisible(bool visible);

	void load(const QDomElement & node, const ModelLoadingContext &ctx);
	bool load(const QJsonObject & obj, const ModelLoadingContext &ctx);
	void save(QJsonObject & obj, const ModelSavingContext &ctx) const;

	void copyFrom(const IndexOfRefractionImpl & srcIOR);

	// IParameterOwner
	CoreInstance & core() const override;
	void pushCommand(QUndoCommand *cmd) override;
	void fireChanged(eParamId paramId) override;
};

inline bool compare(const IndexOfRefractionImpl * ior1, const IndexOfRefractionImpl * ior2)
{
	if (ior1 == ior2)
		return true;

	return ior1 ? ior1->equals(ior2) : ior2->equals(ior1);
}

inline std::complex<float> getIOR(const IndexOfRefractionImpl * ior, float nm = DEFAULT_WAVELENGTH)
{
	return ior ? ior->getIOR(nm) : std::complex<float>(1.f, 0.f);
}
