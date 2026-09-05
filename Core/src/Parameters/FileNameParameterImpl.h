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

class CORE_EXPORT FileNameParameterImpl : public IFileNameParameter
{
	Q_OBJECT

protected:
	const FileFormatsList m_extensions;
	IParameterOwner & m_owner;
	const eParamId m_id;
	QString m_fileName;
	bool    m_enabled;
	bool    m_visible;

public:
	explicit FileNameParameterImpl(const FileFormatsList& exts,
								   eParamId paramId, IParameterOwner & owner);
	~FileNameParameterImpl() override;

	const QString & get() const final override { return m_fileName; }
	void set(const QString &) final override;
	void _set(const QString &);

	const FileFormatsList & extensions() const override { return m_extensions; }

	bool load(const QDomElement & node, const char * name, const ModelLoadingContext &ctx);
	void save(QJsonObject & obj, const char * name, const ModelSavingContext &ctx) const;
	bool load(const QJsonObject & obj, const char * name, const ModelLoadingContext &ctx);

	bool isEnabled() const final override { return m_enabled; }
	void setEnabled(bool b);

	bool isVisible() const final override { return m_visible; }
	void setVisible(bool b);

	void fireChanged(eParamId paramId);
};
