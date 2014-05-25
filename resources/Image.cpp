//
//  Image.cpp
//  MiRay/resources
//
//  Created by Damir Sagidullin on 08.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#include "Image.h"
#include "ImageManager.h"

using namespace mr;

// ------------------------------------------------------------------------ //

inline std::string UniqueImageName(const char * name, void * p)
{
	if (name)
		return name;

	std::ostringstream stringStream;
	stringStream << "Image(" << p << ")";
	return stringStream.str();
}

Image::Image(ImageManager & owner, const char * name)
	: m_owner(owner)
	, m_name(UniqueImageName(name, this))
	, m_width(0)
	, m_height(0)
	, m_pData(NULL)
{
}

Image::~Image()
{
	Destroy();
	m_owner.Release(m_name);
}

// ------------------------------------------------------------------------ //

bool Image::Create(int w, int h)
{
	Destroy();

	m_pData = new byte[w * h * PixelSize()];
	if (!m_pData)
		return false;

	m_width = w;
	m_height = h;
	return true;
}

void Image::Destroy()
{
	if (m_pData)
	{
		delete [] m_pData;
		m_pData = NULL;
	}

	m_width = m_height = 0;
}

// ------------------------------------------------------------------------ //

ColorF Image::GetPixelUV(float u, float v) const
{
	assert(!isnan(u) && !isnan(v));

	if (u > 1.f)
		u = fmodf(u, 1.f);
	else if (u < 0.f)
		u = fmodf(u, 1.f) + 1.f;

	if (v > 1.f)
		v = fmodf(v, 1.f);
	else if (v < 0.f)
		v = fmodf(v, 1.f) + 1.f;

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

	ColorF c11 = GetPixel(ix, iy);
	ColorF c12 = GetPixel(ix2, iy);
	ColorF c21 = GetPixel(ix, iy2);
	ColorF c22 = GetPixel(ix2, iy2);
	ColorF c1 = ColorF::Lerp(c11, c12, dx);
	ColorF c2 = ColorF::Lerp(c21, c22, dx);

	return ColorF::Lerp(c1, c2, dy);
}

Vec3 Image::GetPixelColorUV(float u, float v) const
{
	assert(!isnan(u) && !isnan(v));

	if (u > 1.f)
		u = fmodf(u, 1.f);
	else if (u < 0.f)
		u = fmodf(u, 1.f) + 1.f;
	
	if (v > 1.f)
		v = fmodf(v, 1.f);
	else if (v < 0.f)
		v = fmodf(v, 1.f) + 1.f;
	
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

	Vec3 c11 = GetPixelColor(ix, iy);
	Vec3 c12 = GetPixelColor(ix2, iy);
	Vec3 c21 = GetPixelColor(ix, iy2);
	Vec3 c22 = GetPixelColor(ix2, iy2);
	Vec3 c1 = Vec3::Lerp(c11, c12, dx);
	Vec3 c2 = Vec3::Lerp(c21, c22, dx);
	
	return Vec3::Lerp(c1, c2, dy);
}

float Image::GetPixelOpacityUV(float u, float v) const
{
	assert(!isnan(u) && !isnan(v));

	if (u > 1.f)
		u = fmodf(u, 1.f);
	else if (u < 0.f)
		u = fmodf(u, 1.f) + 1.f;
	
	if (v > 1.f)
		v = fmodf(v, 1.f);
	else if (v < 0.f)
		v = fmodf(v, 1.f) + 1.f;
	
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

	float c11 = GetPixelOpacity(ix, iy);
	float c12 = GetPixelOpacity(ix2, iy);
	float c21 = GetPixelOpacity(ix, iy2);
	float c22 = GetPixelOpacity(ix2, iy2);
	float c1 = lerp(c11, c12, dx);
	float c2 = lerp(c21, c22, dx);

	return lerp(c1, c2, dy);
}

// ------------------------------------------------------------------------ //

void Image1B::SetPixel(int x, int y, const ColorF & c)
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	m_pData[y * m_width + x] = F2B(clamp(c.r, 0.f, 1.f));
}

ColorF Image1B::GetPixel(int x, int y) const
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	float c = B2F(m_pData[y * m_width + x]);
	return ColorF(c, c, c);
}

Vec3 Image1B::GetPixelColor(int x, int y) const
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	float c = B2F(m_pData[y * m_width + x]);
	return Vec3(c, c, c);
}

// ------------------------------------------------------------------------ //

void Image3B::SetPixel(int x, int y, const ColorF & c)
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	byte * pData = m_pData + (y * m_width + x) * 3;
	pData[0] = F2B(clamp(c.r, 0.f, 1.f));
	pData[1] = F2B(clamp(c.g, 0.f, 1.f));
	pData[2] = F2B(clamp(c.b, 0.f, 1.f));
}

ColorF Image3B::GetPixel(int x, int y) const
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	const byte * pData = m_pData + (y * m_width + x) * 3;
	return ColorF(B2F(pData[0]), B2F(pData[1]), B2F(pData[2]));
}

Vec3 Image3B::GetPixelColor(int x, int y) const
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	const byte * pData = m_pData + (y * m_width + x) * 3;
	return Vec3(B2F(pData[0]), B2F(pData[1]), B2F(pData[2]));
}

// ------------------------------------------------------------------------ //

void Image4B::SetPixel(int x, int y, const ColorF & c)
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	byte * pData = m_pData + (y * m_width + x) * 4;
	pData[0] = F2B(clamp(c.r, 0.f, 1.f));
	pData[1] = F2B(clamp(c.g, 0.f, 1.f));
	pData[2] = F2B(clamp(c.b, 0.f, 1.f));
	pData[3] = F2B(clamp(c.a, 0.f, 1.f));
}

ColorF Image4B::GetPixel(int x, int y) const
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	const byte * pData = m_pData + (y * m_width + x) * 4;
	return ColorF(B2F(pData[0]), B2F(pData[1]), B2F(pData[2]), B2F(pData[3]));
}

Vec3 Image4B::GetPixelColor(int x, int y) const
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	const byte * pData = m_pData + (y * m_width + x) * 4;
	return Vec3(B2F(pData[0]), B2F(pData[1]), B2F(pData[2]));
}

float Image4B::GetPixelOpacity(int x, int y) const
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	return B2F(m_pData[(y * m_width + x) * 4 + 3]);
}

// ------------------------------------------------------------------------ //

void Image1W::SetPixel(int x, int y, const ColorF & c)
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	reinterpret_cast<uint16 *>(m_pData)[y * m_width + x] = F2W(clamp(c.r, 0.f, 1.f));
}

ColorF Image1W::GetPixel(int x, int y) const
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	float c = W2F(reinterpret_cast<uint16 *>(m_pData)[y * m_width + x]);
	return ColorF(c, c, c);
}

Vec3 Image1W::GetPixelColor(int x, int y) const
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	float c = W2F(reinterpret_cast<uint16 *>(m_pData)[y * m_width + x]);
	return Vec3(c, c, c);
}

// ------------------------------------------------------------------------ //

void Image3W::SetPixel(int x, int y, const ColorF & c)
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	uint16 * pData = reinterpret_cast<uint16 *>(m_pData) + (y * m_width + x) * 3;
	pData[0] = F2W(clamp(c.r, 0.f, 1.f));
	pData[1] = F2W(clamp(c.g, 0.f, 1.f));
	pData[2] = F2W(clamp(c.b, 0.f, 1.f));
}

ColorF Image3W::GetPixel(int x, int y) const
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	const uint16 * pData = reinterpret_cast<uint16 *>(m_pData) + (y * m_width + x) * 3;
	return ColorF(W2F(pData[0]), W2F(pData[1]), W2F(pData[2]));
}

Vec3 Image3W::GetPixelColor(int x, int y) const
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	const uint16 * pData = reinterpret_cast<uint16 *>(m_pData) + (y * m_width + x) * 3;
	return Vec3(W2F(pData[0]), W2F(pData[1]), W2F(pData[2]));
}

// ------------------------------------------------------------------------ //

void Image4W::SetPixel(int x, int y, const ColorF & c)
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	uint16 * pData = reinterpret_cast<uint16 *>(m_pData) + (y * m_width + x) * 4;
	pData[0] = F2W(clamp(c.r, 0.f, 1.f));
	pData[1] = F2W(clamp(c.g, 0.f, 1.f));
	pData[2] = F2W(clamp(c.b, 0.f, 1.f));
	pData[3] = F2W(clamp(c.a, 0.f, 1.f));
}

ColorF Image4W::GetPixel(int x, int y) const
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	const uint16 * pData = reinterpret_cast<uint16 *>(m_pData) + (y * m_width + x) * 4;
	return ColorF(W2F(pData[0]), W2F(pData[1]), W2F(pData[2]), W2F(pData[3]));
}

Vec3 Image4W::GetPixelColor(int x, int y) const
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	const uint16 * pData = reinterpret_cast<uint16 *>(m_pData) + (y * m_width + x) * 4;
	return Vec3(W2F(pData[0]), W2F(pData[1]), W2F(pData[2]));
}

float Image4W::GetPixelOpacity(int x, int y) const
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	return W2F(reinterpret_cast<uint16 *>(m_pData)[(y * m_width + x) * 4 + 3]);
}

// ------------------------------------------------------------------------ //

void Image1F::SetPixel(int x, int y, const ColorF & c)
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	reinterpret_cast<float *>(m_pData)[y * m_width + x] = c.r;
}

ColorF Image1F::GetPixel(int x, int y) const
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	float c = reinterpret_cast<float *>(m_pData)[y * m_width + x];
	return ColorF(c, c, c);
}

Vec3 Image1F::GetPixelColor(int x, int y) const
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	float c = reinterpret_cast<float *>(m_pData)[y * m_width + x];
	return Vec3(c, c, c);
}

// ------------------------------------------------------------------------ //

void Image3F::SetPixel(int x, int y, const ColorF & c)
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	float * pData = reinterpret_cast<float *>(m_pData) + (y * m_width + x) * 3;
	pData[0] = c.r;
	pData[1] = c.g;
	pData[2] = c.b;
}

ColorF Image3F::GetPixel(int x, int y) const
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	const float * pData = reinterpret_cast<float *>(m_pData) + (y * m_width + x) * 3;
	return ColorF(pData[0], pData[1], pData[2]);
}

Vec3 Image3F::GetPixelColor(int x, int y) const
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	const float * pData = reinterpret_cast<float *>(m_pData) + (y * m_width + x) * 3;
	return Vec3(pData[0], pData[1], pData[2]);
}

// ------------------------------------------------------------------------ //

void Image4F::SetPixel(int x, int y, const ColorF & c)
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	float * pData = reinterpret_cast<float *>(m_pData) + (y * m_width + x) * 4;
	pData[0] = c.r;
	pData[1] = c.g;
	pData[2] = c.b;
	pData[3] = c.a;
}

ColorF Image4F::GetPixel(int x, int y) const
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	const float * pData = reinterpret_cast<float *>(m_pData) + (y * m_width + x) * 4;
	return ColorF(pData[0], pData[1], pData[2], pData[3]);
}

Vec3 Image4F::GetPixelColor(int x, int y) const
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	const float * pData = reinterpret_cast<float *>(m_pData) + (y * m_width + x) * 4;
	return Vec3(pData[0], pData[1], pData[2]);
}

float Image4F::GetPixelOpacity(int x, int y) const
{
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	return reinterpret_cast<float *>(m_pData)[(y * m_width + x) * 4 + 3];
}

// ------------------------------------------------------------------------ //
