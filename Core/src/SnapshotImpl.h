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

#include "ParametersImpl.h"

class SnapshotImpl final : public ISnapshot, public IParameterOwner
{
	Scene &					m_scene;
	QString					m_guid;
	QJsonObject				m_state;
	QImage					m_preview;
	StringParameterImpl		m_name;
	BooleanParameterImpl	m_hasCameraState;
	BooleanParameterImpl	m_hasVisibility;
	BooleanParameterImpl	m_hasTransformations;
	BooleanParameterImpl	m_hasAssignedMaterials;
	BooleanParameterImpl	m_hasEnvironment;
	BooleanParameterImpl	m_hasBackground;

public:
	SnapshotImpl(Scene & scene, const QString & name);
	~SnapshotImpl();

	void _setState(const QJsonObject & state) { m_state = state; }
	void _setPreview(const QImage & image) { m_preview = image; }

	StringParameterImpl & name() override { return m_name; }

	SnapshotParams	getParameters() const override;

	const QString & guid() const { return m_guid; }
	void _setGuid(const QString & guid) { m_guid = guid; }

	QImage getPreview() const override { return m_preview; }
	const QJsonObject & getState() const { return m_state; }

	void activate() const override;
	void updateWithCurrent() override;

	uint32_t getFlags() const;

	BooleanParameterImpl & hasCameraState() override { return m_hasCameraState; }
	BooleanParameterImpl & hasVisibility() override { return m_hasVisibility; }
	BooleanParameterImpl & hasTransformations() override { return m_hasTransformations; }
	BooleanParameterImpl & hasAssignedMaterials() override { return m_hasAssignedMaterials; }
	BooleanParameterImpl & hasEnvironment() override { return m_hasEnvironment; }
	BooleanParameterImpl & hasBackground() override { return m_hasBackground; }

	CoreInstance & core() const override;
	void pushCommand(QUndoCommand *cmd) override;
	void fireChanged(eParamId paramId) override;
};

using SnapshotPtr = std::unique_ptr<SnapshotImpl>;
using SnapshotsList = std::vector<SnapshotPtr>;
