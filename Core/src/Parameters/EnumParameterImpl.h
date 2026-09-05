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

class CORE_EXPORT EnumParameterImpl : public IEnumParameter
{
	Q_OBJECT

protected:
	IParameterOwner & m_owner;
	const eParamId m_id;
	const char * const * m_items;
	const int m_count;
	bool m_enabled;
	bool m_visible;
	int  m_index;

public:
	explicit EnumParameterImpl(int index, const char * const * items, eParamId paramId, IParameterOwner & owner);
	~EnumParameterImpl();

	int count() const final override { return m_count; }
	QString getTitle(int i) const final override { return m_items[i]; }

	int getIndex() const final override { return m_index; }
	void setIndex(int) final override;
	void _setIndex(int) final override;

	bool load(const QDomElement & node, const char * name);
	void save(QJsonObject & obj, const char * name) const;
	bool load(const QJsonObject & obj, const char * name);

	bool isEnabled() const final override { return m_enabled; }
	void setEnabled(bool b);

	bool isVisible() const final override { return m_visible; }
	void setVisible(bool b);

	void fireChanged(eParamId paramId);
};
