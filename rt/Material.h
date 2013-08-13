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

enum ePixelFormat
{
	PIXEL_FORMAT_NONE,
	PIXEL_FORMAT_1B,
	PIXEL_FORMAT_3B,
	PIXEL_FORMAT_4B,
	PIXEL_FORMAT_1F,
	PIXEL_FORMAT_3F,
	PIXEL_FORMAT_4F,
};

class MaterialParameter
{
	Vec3			m_color;

	int				m_width;
	int				m_height;
	int				m_pitch;
	ePixelFormat	m_pixelFormat;
	const void *	m_pData;

	float GetTextureValue(int x, int y) const;
	Vec3 GetTextureColor(int x, int y) const;

public:
	MaterialParameter(const Vec3 & c);

	float Value(const Vec2 & tc) const;
	Vec3 Color(const Vec2 & tc) const;

	void SetColor(const Vec3 & c) { m_color = c; }
	const Vec3 & GetColor() const { return m_color; }

	void SetTexture(int width, int height, ePixelFormat pf, int pitch, const void * pData);
	bool HasTexture() const { return NULL != m_pData; }
	int TextureWidth() const { return m_width; }
	int TextureHeight() const { return m_height; }
	int TexturePitch() const { return m_pitch; }
	ePixelFormat PixelFormat() const { return m_pixelFormat; }
	const void * TextureData() const { return m_pData; }
};

class MaterialLayer
{
protected:
	MaterialParameter	m_ambient;
	MaterialParameter	m_emissive;
	MaterialParameter	m_diffuse;
	MaterialParameter	m_opacity;
	MaterialParameter	m_indexOfRefraction;

	bool m_fresnelReflection;
	MaterialParameter	m_reflection;
	MaterialParameter	m_reflectionTint;
	MaterialParameter	m_reflectionRoughness;
	MaterialParameter	m_reflectionExitColor;

	MaterialParameter	m_refractionTint;
	MaterialParameter	m_refractionRoughness;
	MaterialParameter	m_refractionExitColor;

	MaterialParameter	m_bumpLevel;
	MaterialParameter	m_normalmap;

	virtual ~MaterialLayer() {}

public:
	MaterialLayer();

	const MaterialParameter & Ambient() const { return m_ambient; }

	const MaterialParameter & Emissive() const { return m_emissive; }

	const MaterialParameter & Diffuse() const { return m_diffuse; }

	const MaterialParameter & Opacity() const { return m_opacity; }
	const MaterialParameter & IndexOfRefraction() const { return m_indexOfRefraction; }

	bool FresnelReflection() const { return m_fresnelReflection; }
	const MaterialParameter & Reflection() const { return m_reflection; }
	const MaterialParameter & ReflectionTint() const { return m_reflectionTint; }
	const MaterialParameter & ReflectionRoughness() const { return m_reflectionRoughness; }
	const MaterialParameter & ReflectionExitColor() const { return m_reflectionExitColor; }

	const MaterialParameter & RefractionTint() const { return m_refractionTint; }
	const MaterialParameter & RefractionRoughness() const { return m_refractionRoughness; }
	const MaterialParameter & RefractionExitColor() const { return m_refractionExitColor; }
	
	const MaterialParameter & BumpLevel() const { return m_bumpLevel; }
	const MaterialParameter & Normalmap() const { return m_normalmap; }
};

class IMaterial
{
protected:
	virtual ~IMaterial() {}

public:
	virtual size_t NumLayers() const = 0;
	virtual const MaterialLayer * Layer(size_t i) const = 0;
};

}