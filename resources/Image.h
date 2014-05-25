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
		TYPE_1W,
		TYPE_3W,
		TYPE_4W,
		TYPE_1F,
		TYPE_3F,
		TYPE_4F,
	};

protected:
	ImageManager & m_owner;

	const std::string m_name;

	int		m_width;
	int		m_height;
	byte *	m_pData;

	Image(ImageManager & owner, const char * name);

public:
	virtual ~Image();

	bool Create(int w, int h);
	void Destroy();

	ColorF GetPixelUV(float u, float v) const;

	Vec3 GetPixelColorUV(float u, float v) const;
	float GetPixelOpacityUV(float u, float v) const;

	const std::string & Name() const { return m_name; }

	int Width() const { return m_width; }
	int Height() const { return m_height; }
	virtual eType Type() const  = 0;
	virtual int PixelSize() const  = 0;
	virtual int NumChannels() const = 0;

	const void * Data() const { return m_pData; }
	void * Data() { return m_pData; }

	const byte * DataB() const { return m_pData; }
	byte * DataB() { return m_pData; }

	const uint16 * DataW() const { return reinterpret_cast<uint16 *>(m_pData); }
	uint16 * DataW() { return reinterpret_cast<uint16 *>(m_pData); }

	const float * DataF() const { return reinterpret_cast<float *>(m_pData); }
	float * DataF() { return reinterpret_cast<float *>(m_pData); }
};

class Image1B : public Image
{
public:
	Image1B(ImageManager & owner, const char * name) : Image(owner, name) {}
	
	eType Type() const { return Image::TYPE_1B; }
	int PixelSize() const { return 1; }
	int NumChannels() const { return 1; }
	
	void SetPixel(int x, int y, const ColorF & c);
	ColorF GetPixel(int x, int y) const;
	
	Vec3 GetPixelColor(int x, int y) const;
	float GetPixelOpacity(int x, int y) const { return 1.f; }
};

class Image3B : public Image
{
public:
	Image3B(ImageManager & owner, const char * name) : Image(owner, name) {}
	
	eType Type() const { return Image::TYPE_3B; }
	int PixelSize() const { return 3; }
	int NumChannels() const { return 3; }
	
	void SetPixel(int x, int y, const ColorF & c);
	ColorF GetPixel(int x, int y) const;
	
	Vec3 GetPixelColor(int x, int y) const;
	float GetPixelOpacity(int x, int y) const { return 1.f; }
};

class Image4B : public Image
{
public:
	Image4B(ImageManager & owner, const char * name) : Image(owner, name) {}
	
	eType Type() const { return Image::TYPE_4B; }
	int PixelSize() const { return 4; }
	int NumChannels() const { return 4; }
	
	void SetPixel(int x, int y, const ColorF & c);
	ColorF GetPixel(int x, int y) const;
	
	Vec3 GetPixelColor(int x, int y) const;
	float GetPixelOpacity(int x, int y) const;
};

class Image1W : public Image
{
public:
	Image1W(ImageManager & owner, const char * name) : Image(owner, name) {}
	
	eType Type() const { return Image::TYPE_1W; }
	int PixelSize() const { return 2; }
	int NumChannels() const { return 1; }

	void SetPixel(int x, int y, const ColorF & c);
	ColorF GetPixel(int x, int y) const;
	
	Vec3 GetPixelColor(int x, int y) const;
	float GetPixelOpacity(int x, int y) const { return 1.f; }
};
	
class Image3W : public Image
{
public:
	Image3W(ImageManager & owner, const char * name) : Image(owner, name) {}
	
	eType Type() const { return Image::TYPE_3W; }
	int PixelSize() const { return 6; }
	int NumChannels() const { return 3; }
	
	void SetPixel(int x, int y, const ColorF & c);
	ColorF GetPixel(int x, int y) const;

	Vec3 GetPixelColor(int x, int y) const;
	float GetPixelOpacity(int x, int y) const { return 1.f; }
};

class Image4W : public Image
{
public:
	Image4W(ImageManager & owner, const char * name) : Image(owner, name) {}
	
	eType Type() const { return Image::TYPE_4W; }
	int PixelSize() const { return 8; }
	int NumChannels() const { return 4; }
	
	void SetPixel(int x, int y, const ColorF & c);
	ColorF GetPixel(int x, int y) const;
	
	Vec3 GetPixelColor(int x, int y) const;
	float GetPixelOpacity(int x, int y) const;
};

class Image1F : public Image
{
public:
	Image1F(ImageManager & owner, const char * name) : Image(owner, name) {}

	eType Type() const { return Image::TYPE_1F; }
	int PixelSize() const { return 4; }
	int NumChannels() const { return 1; }

	void SetPixel(int x, int y, const ColorF & c);
	ColorF GetPixel(int x, int y) const;

	Vec3 GetPixelColor(int x, int y) const;
	float GetPixelOpacity(int x, int y) const { return 1.f; }
};

class Image3F : public Image
{
public:
	Image3F(ImageManager & owner, const char * name) : Image(owner, name) {}
	
	eType Type() const { return Image::TYPE_3F; }
	int PixelSize() const { return 12; }
	int NumChannels() const { return 3; }
	
	void SetPixel(int x, int y, const ColorF & c);
	ColorF GetPixel(int x, int y) const;
	
	Vec3 GetPixelColor(int x, int y) const;
	float GetPixelOpacity(int x, int y) const { return 1.f; }
};

class Image4F : public Image
{
public:
	Image4F(ImageManager & owner, const char * name) : Image(owner, name) {}
	
	eType Type() const { return Image::TYPE_4F; }
	int PixelSize() const { return 16; }
	int NumChannels() const { return 4; }
	
	void SetPixel(int x, int y, const ColorF & c);
	ColorF GetPixel(int x, int y) const;
	
	Vec3 GetPixelColor(int x, int y) const;
	float GetPixelOpacity(int x, int y) const;
};

}