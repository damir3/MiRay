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

struct GeomInfo {
	Geometry *			geom;

	GeomData			data1;
	GeomData			data2;
};

class UpdateGeometriesCommand : public QUndoCommand
{
	Scene & m_scene;
	std::vector<GeomInfo>	m_geoms;
	std::vector<MeshNode *>	m_meshes;
	bool	m_firstTime;

	void redo() override;
	void undo() override;

public:
	UpdateGeometriesCommand(Scene & scene, std::vector<GeomInfo> & geoms, std::set<MeshNode *> & meshes)
		: m_scene(scene)
		, m_geoms(geoms)
		, m_meshes(meshes.begin(), meshes.end())
		, m_firstTime(true)
	{
	}
};

class UVMapping final : public IUVMapping, public IParameterOwner
{
	Scene &					m_scene;
	std::vector<GeomInfo>	m_geoms;
	std::set<MeshNode *>	m_meshes;
	EnumParameterImpl		m_mapping;
	EnumParameterImpl		m_fitTo;
	EnumParameterImpl		m_uvSet;
	BooleanParameterImpl	m_flipU;
	BooleanParameterImpl	m_flipV;
	BooleanParameterImpl	m_normalize;
	Vec3ParameterImpl		m_scale;
	Vec2ParameterImpl		m_repeat;
	int						m_uvsetIndex;
	bool					m_accept;

	void updateBBox(BBox & bbox, const vec3 & nodeScale);
	void restore();
	void doIt();

public:
	UVMapping(Scene & scene, const GeomSelection &meshes);
	~UVMapping() override;

	EnumParameterImpl &mapping() override { return m_mapping; }
	EnumParameterImpl &fitTo() override { return m_fitTo; }
	EnumParameterImpl &uvset() override { return m_uvSet; }
	BooleanParameterImpl &flipU() override { return m_flipU; }
	BooleanParameterImpl &flipV() override { return m_flipV; }
	BooleanParameterImpl &normalize() override { return m_normalize; }
	Vec3ParameterImpl &scale() override { return m_scale; }
	Vec2ParameterImpl &repeat() override { return m_repeat; }

	void accept() override;

	CoreInstance & core() const override;
	void pushCommand(QUndoCommand *cmd) override;
	void fireChanged(eParamId paramId) override;
};
