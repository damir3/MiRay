//
//  Material.cpp
//  MiRay/rt
//
//  Created by Damir Sagidullin on 26.06.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#include "Material.h"

using namespace mr;

// ------------------------------------------------------------------------ //

MaterialLayer::MaterialLayer()
	: m_ambient(ColorF::Null)
	, m_emissive(ColorF::Null)
	, m_diffuse(ColorF::White)
	, m_opacity(ColorF::White)
	, m_indexOfRefraction(ColorF(1.5f))
	, m_fresnelReflection(false)
	, m_reflection(ColorF::Null)
	, m_reflectionTint(ColorF::White)
	, m_reflectionGlossiness(ColorF::White)
	, m_reflectionExitColor(ColorF::Null)
	, m_refractionTint(ColorF::White)
	, m_refractionGlossiness(ColorF::White)
	, m_refractionExitColor(ColorF::Null)
	, m_bumpLevel(ColorF::White)
	, m_normalmap(ColorF::White)
{
}

// ------------------------------------------------------------------------ //

MaterialParameter::MaterialParameter(const ColorF & c)
	: m_color(c)
	, m_width(0)
	, m_height(0)
	, m_pixelFormat(PIXEL_FORMAT_NONE)
	, m_pitch(0)
	, m_pData(NULL)
{
	assert(!m_pData);
}

void MaterialParameter::SetTexture(int width, int height, ePixelFormat pf, int pitch, const void * pData)
{
	m_width = width;
	m_height = height;
	m_pixelFormat = pf;
	m_pitch = pitch;
	m_pData = pData;
}

float MaterialParameter::GetTextureValue(int x, int y) const
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	switch (m_pixelFormat)
	{
		case PIXEL_FORMAT_1B:
			return B2F(reinterpret_cast<const byte *>(m_pData)[y * m_pitch + x]);
		case PIXEL_FORMAT_3B:
			return B2F(reinterpret_cast<const byte *>(m_pData)[y * m_pitch + x * 3]);
		case PIXEL_FORMAT_4B:
			return B2F(reinterpret_cast<const byte *>(m_pData)[y * m_pitch + x * 4]);
		case PIXEL_FORMAT_1F:
			return reinterpret_cast<const float *>(m_pData)[y * m_pitch + x];
		case PIXEL_FORMAT_3F:
			return reinterpret_cast<const float *>(m_pData)[y * m_pitch + x * 3];
		case PIXEL_FORMAT_4F:
			return reinterpret_cast<const float *>(m_pData)[y * m_pitch + x * 4];
		default:
			return 0.f;
	}
}

ColorF MaterialParameter::GetTextureColor(int x, int y) const
{
	assert(m_pData);
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	switch (m_pixelFormat)
	{
		case PIXEL_FORMAT_1B:
		{
			float f = B2F(reinterpret_cast<const byte *>(m_pData)[y * m_pitch + x]);
			return ColorF(f, f, f, f);
		}
		case PIXEL_FORMAT_3B:
		{
			const byte * pData = reinterpret_cast<const byte *>(m_pData) + y * m_pitch + x * 3;
			return ColorF(B2F(pData[0]), B2F(pData[1]), B2F(pData[2]));
		}
		case PIXEL_FORMAT_4B:
		{
			const byte * pData = reinterpret_cast<const byte *>(m_pData) + y * m_pitch + x * 4;
			return ColorF(B2F(pData[0]), B2F(pData[1]), B2F(pData[2]), B2F(pData[3]));
		}
		case PIXEL_FORMAT_1F:
		{
			float f = reinterpret_cast<const float *>(m_pData)[y * m_pitch + x];
			return ColorF(f, f, f, f);
		}
		case PIXEL_FORMAT_3F:
		{
			const float * pData = reinterpret_cast<const float *>(m_pData) + y * m_pitch + x * 3;
			return ColorF(pData[0], pData[1], pData[2]);
		}
		case PIXEL_FORMAT_4F:
		{
			const float * pData = reinterpret_cast<const float *>(m_pData) + y * m_pitch + x * 4;
			return ColorF(pData[0], pData[1], pData[2], pData[3]);
		}
		default:
			return ColorF::Null;
	}
}

float MaterialParameter::Value(const Vec2 & tc) const
{
	if (!m_pData)
		return m_color.r;

	float u, v;
	if (tc.x > 1.f)
		u = fmodf(tc.x, 1.f);
	else if (tc.x < 0.f)
		u = fmodf(tc.x, 1.f) + 1.f;
	else
		u = tc.x;
	
	if (tc.y > 1.f)
		v = fmodf(tc.y, 1.f);
	else if (tc.y < 0.f)
		v = fmodf(tc.y, 1.f) + 1.f;
	else
		v = tc.y;


	float fx = u * m_width - 0.5f;
	float fy = v * m_height - 0.5f;

	int ix = static_cast<int>(fx);
	int iy = static_cast<int>(fy);
	float dx = fx - ix;
	float dy = fy - iy;
	int ix2 = ix + 1;
	int iy2 = iy + 1;

	if (ix < 0)				ix += m_width;
	if (ix2 >= m_width)		ix2 -= m_width;
	if (iy < 0)				iy += m_height;
	if (iy2 >= m_height)	iy2 -= m_height;

	float c11 = GetTextureValue(ix, iy);
	float c12 = GetTextureValue(ix2, iy);
	float c21 = GetTextureValue(ix, iy2);
	float c22 = GetTextureValue(ix2, iy2);
	float c1 = lerp<float>(c11, c12, dx);
	float c2 = lerp<float>(c21, c22, dx);

	return lerp<float>(c1, c2, dy) * m_color.r;
}

ColorF MaterialParameter::Color(const Vec2 & tc) const
{
	if (!m_pData)
		return m_color;

	float u, v;
	if (tc.x > 1.f)
		u = fmodf(tc.x, 1.f);
	else if (tc.x < 0.f)
		u = fmodf(tc.x, 1.f) + 1.f;
	else
		u = tc.x;

	if (tc.y > 1.f)
		v = fmodf(tc.y, 1.f);
	else if (tc.y < 0.f)
		v = fmodf(tc.y, 1.f) + 1.f;
	else
		v = tc.y;

	float fx = u * m_width - 0.5f;
	float fy = v * m_height - 0.5f;

	int ix = static_cast<int>(fx);
	int iy = static_cast<int>(fy);
	float dx = fx - ix;
	float dy = fy - iy;
	int ix2 = ix + 1;
	int iy2 = iy + 1;

	if (ix < 0)				ix += m_width;
	if (ix2 >= m_width)		ix2 -= m_width;
	if (iy < 0)				iy += m_height;
	if (iy2 >= m_height)	iy2 -= m_height;

	ColorF c11 = GetTextureColor(ix, iy);
	ColorF c12 = GetTextureColor(ix2, iy);
	ColorF c21 = GetTextureColor(ix, iy2);
	ColorF c22 = GetTextureColor(ix2, iy2);
	ColorF c1 = ColorF::Lerp(c11, c12, dx);
	ColorF c2 = ColorF::Lerp(c21, c22, dx);

	return ColorF::Lerp(c1, c2, dy) * m_color;
}

