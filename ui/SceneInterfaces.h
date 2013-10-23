//
//  SceneInterfaces.h
//  MiRay/ui
//
//  Created by Damir Sagidullin on 25.08.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

#include "../rt/Light.h"

namespace mr
{

class ITransformable
{
public:
	virtual ~ITransformable() {}
	
	virtual BBoxF OOBB() const = 0;
	virtual Matrix Transformation() const = 0;
	virtual void SetTransformation(const Matrix & mat) = 0;
};

class ISceneLight : public ILight, public ITransformable
{
public:
	virtual bool Load(pugi::xml_node node) = 0;
	virtual void Save(pugi::xml_node node) const = 0;

	virtual bool TraceRay(const Vec3 & vFrom, const Vec3 & vTo, TraceResult & tr) const = 0;
	virtual void Draw() = 0;
};

}