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
#include "UVMapping.h"

class EditNormals final : public IEditNormals, public IParameterOwner
{
	Scene &					m_scene;
	std::vector<GeomInfo>	m_geoms;
	std::set<MeshNode *>	m_meshes;
	BooleanParameterImpl	m_calculateNormals;
	EnumParameterImpl		m_makeEdges;
	ScalarParameterImpl		m_maxSoftAngle;
	BooleanParameterImpl	m_flipNormals;
	BooleanParameterImpl	m_flipFacing;
	bool					m_accept;

	void restore();
	void doIt();

public:
	EditNormals(Scene & scene, const GeomSelection &meshes);
	~EditNormals() override;

	BooleanParameterImpl &calculateNormals() override { return m_calculateNormals; }
	EnumParameterImpl &makeEdges() override { return m_makeEdges; }
	ScalarParameterImpl &maxSoftAngle() override { return m_maxSoftAngle; }
	BooleanParameterImpl &flipNormals() override { return m_flipNormals; }
	BooleanParameterImpl &flipFacing() override { return m_flipFacing; }

	void accept() override;

	CoreInstance & core() const override;
	void pushCommand(QUndoCommand *cmd) override;
	void fireChanged(eParamId paramId) override;
};
