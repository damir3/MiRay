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

class FitToView final : public IFitToView, public IParameterOwner
{
	Scene &					m_scene;
	Camera &				m_camera;
	CameraState				m_state;
	BooleanParameterImpl	m_justSelectedObjects;
	BooleanParameterImpl	m_includingChildrenObjects;
	BooleanParameterImpl	m_keepAspect;
	ScalarParameterImpl		m_padding;
	bool					m_accept;
	float					m_distance;
	float					m_aspect;

	void doIt();

public:
	FitToView(Scene & scene);
	~FitToView() override;

	BooleanParameterImpl &justSelectedObjects() override { return m_justSelectedObjects; }
	BooleanParameterImpl &includingChildrenObjects() override { return m_includingChildrenObjects; }
	BooleanParameterImpl &keepAspect() override { return m_keepAspect; }
	ScalarParameterImpl &padding() override { return m_padding; }

	void accept() override;

	CoreInstance & core() const override;
	void pushCommand(QUndoCommand *cmd) override;
	void fireChanged(eParamId paramId) override;
};
