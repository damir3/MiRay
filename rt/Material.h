//
//  Material.h
//  MiRay/rt
//
//  Created by Damir Sagidullin on 26.06.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

namespace mr
{

struct MaterialContext
{
	Vec2 tc;
	Vec3 dir;
	Vec3 normal;
	Vec3 tangent;
	Vec3 binormal;
};

class IMaterialLayer
{
protected:
	virtual ~IMaterialLayer() {}
	
public:
	virtual Vec3 Ambient(const MaterialContext & mc) const = 0;
	virtual Vec3 Emissive(const MaterialContext & mc) const = 0;
	virtual Vec3 Diffuse(const MaterialContext & mc) const = 0;
	virtual Vec3 Opacity(const MaterialContext & mc) const = 0;
	virtual Vec3 IndexOfRefraction() const = 0;
	
	virtual bool FresnelReflection() const = 0;
	virtual bool RaytracedReflection() const = 0;
	
	virtual Vec3 Reflection(const MaterialContext & mc) const = 0;
	virtual Vec3 ReflectionTint(const MaterialContext & mc) const = 0;
	virtual float ReflectionRoughness(const MaterialContext & mc) const = 0;
	virtual bool HasReflectionExitColor() const = 0;
	virtual Vec3 ReflectionExitColor(const MaterialContext & mc) const = 0;
	virtual bool HasReflectionMap() const = 0;
	virtual Vec3 ReflectionMap(const MaterialContext & mc) const = 0;
	
	virtual Vec3 RefractionTint(const MaterialContext & mc) const = 0;
	virtual float RefractionRoughness(const MaterialContext & mc) const = 0;
	virtual bool HasRefractionExitColor() const = 0;
	virtual Vec3 RefractionExitColor(const MaterialContext & mc) const = 0;

	virtual Vec3 AbsorbtionCoefficient() const = 0;

	virtual bool HasBumpMap() const = 0;
	virtual float BumpDepth() const = 0;
	virtual float BumpMapDepth(const Vec2 &tc) const = 0;
	virtual Vec3 BumpMapNormal(const MaterialContext & mc) const = 0;
};

class IMaterial
{
protected:
	virtual ~IMaterial() {}
	
public:
	virtual size_t NumLayers() const = 0;
	virtual const IMaterialLayer * Layer(size_t i) const = 0;
};

}