//
//  Light.h
//  MiRay/rt
//
//  Created by Damir Sagidullin on 11.08.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

namespace mr
{

class ILight
{
public:
	virtual ~ILight() {}

	virtual Vec3 Position(const Vec3 & p) const = 0;
	virtual Vec3 Intensity(float squared_distance) const = 0; // diffuse
	virtual Vec3 Intensity(const Vec3 & rayPos, const Vec3 & rayDir) const = 0; // specular
};

}