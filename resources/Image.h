//
//  Image.h
//  MiRay/resources
//
//  Created by Damir Sagidullin on 03.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

#include "../rt/Image.h"

namespace mr
{

class ImageManager;

class Image : public IImage
{
public:
	enum eType
	{
		TYPE_NONE,
		TYPE_1B,
		TYPE_3B,
		TYPE_4B,
		TYPE_3W,
		TYPE_4W,
		TYPE_1F,
		TYPE_3F,
		TYPE_4F,
	};

	static int PixelSize(eType type)
	{
		switch (type)
		{
			case TYPE_1B: return 1;
			case TYPE_3B: return 3;
			case TYPE_4B: return 4;
			case TYPE_3W: return 6;
			case TYPE_4W: return 8;
			case TYPE_1F: return 4;
			case TYPE_3F: return 12;
			case TYPE_4F: return 16;
			default: return 0;
		}
	}

protected:
	friend ImageManager;
	ImageManager & m_owner;

	const std::string m_name;

	int		m_width;
	int		m_height;
	eType	m_type;
	byte *	m_pData;

	Image(ImageManager & owner, const char * name);

public:
	virtual ~Image();

	bool Create(int w, int h, eType type);
	void Destroy();

	void SetPixel(int x, int y, const ColorF & c);
	ColorF GetPixel(int x, int y) const;
	ColorF GetPixel(float u, float v) const;

	Vec3 GetPixelColor(int x, int y) const;
	Vec3 GetPixelColor(float u, float v) const;

	float GetPixelOpacity(int x, int y) const;
	float GetPixelOpacity(float u, float v) const;

	const std::string & Name() const { return m_name; }

	int Width() const { return m_width; }
	int Height() const { return m_height; }
	eType Type() const { return m_type; }
	int PixelSize() const { return PixelSize(m_type); }
	int NumChannels() const
	{
		switch (m_type)
		{
			case TYPE_3B:
			case TYPE_3W:
			case TYPE_3F:
				return 3;
			case TYPE_4B:
			case TYPE_4W:
			case TYPE_4F:
				return 4;
			default:
				return 0;
		}
	}

	const void * Data() const { return m_pData; }
	void * Data() { return m_pData; }

	const byte * DataB() const { return m_pData; }
	byte * DataB() { return m_pData; }

	const uint16 * DataW() const { return reinterpret_cast<uint16 *>(m_pData); }
	uint16 * DataW() { return reinterpret_cast<uint16 *>(m_pData); }

	const float * DataF() const { return reinterpret_cast<float *>(m_pData); }
	float * DataF() { return reinterpret_cast<float *>(m_pData); }
};

}