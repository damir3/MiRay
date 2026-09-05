/*
	Copyright (C) 2018-2020 Damir Sagidullin

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

#include "ParametersImpl.h"
#include "../../Shared/Interfaces/RenderLayer.h"

class RenderLayerImpl;
class RenderLayerManager;

class RenderLayerEnumParamImpl final : public IEnumParameter
{
	Q_OBJECT

protected:
	RenderLayerManager & m_manager;
	IParameterOwner &	m_owner;
	const char *		m_name;
	RenderLayerImpl *	m_value;
	eParamId			m_id;
	bool				m_visible;

private slots:
	void onListChanged();
	void onLayerChanged(IRenderLayer *);

public:
	RenderLayerEnumParamImpl(RenderLayerManager &manager, eParamId id, IParameterOwner & owner);
	~RenderLayerEnumParamImpl() override;

	int count() const override;
	QString getTitle(int i) const override;

	int getIndex() const override;
	void setIndex(int) override;
	void _setIndex(int) override;

	const RenderLayerImpl * getValue() const { return m_value; }
	void _setValue(RenderLayerImpl * value);

	bool isEnabled() const override { return true; }

	bool isVisible() const override { return m_visible; }
	void setVisible(bool b);

	void fireChanged(eParamId paramId);
};

class RenderLayerImpl final : public IRenderLayer, public IParameterOwner, public IStringParameterValidator
{
	Scene &				m_scene;
	int					m_index;
	StringParameterImpl	m_name;

	QString validate(IStringParameter *property, QString val) const override;

public:
	RenderLayerImpl(Scene & scene, const QString & name);

	StringParameterImpl & name() override { return m_name; }

	int getIndex() const { return m_index; }
	void _setIndex(int index) { m_index = index; }

	CoreInstance & core() const override;
	void pushCommand(QUndoCommand *cmd) override;
	void fireChanged(eParamId paramId) override;
};
