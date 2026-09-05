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

class SnapshotInterpolator final : public ISnapshotInterpolator, public IParameterOwner
{
	Scene &					m_scene;
	const QJsonObject 		m_defaultState;
	uint32_t 				m_defaultStateFlags;
	IntegerParameterImpl	m_transitionFrame;
	IntegerParameterImpl	m_transitionFrameCount;
	ScalarParameterImpl		m_transitionExponent;
	ScalarParameterImpl		m_motionBlur;

public:
	SnapshotInterpolator(Scene & scene, const ISnapshot & snapshot1, const ISnapshot & snapshot2);
	~SnapshotInterpolator();

	IntegerParameterImpl & transitionFrame() override { return m_transitionFrame; }
	IntegerParameterImpl & transitionFrameCount() override { return m_transitionFrameCount; }
	ScalarParameterImpl & transitionExponent() override { return m_transitionExponent; }
	ScalarParameterImpl & motionBlur() override { return m_motionBlur; }

	CoreInstance & core() const override;
	void pushCommand(QUndoCommand *cmd) override;
	void fireChanged(eParamId paramId) override;
};
