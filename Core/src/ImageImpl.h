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

#include "../../Shared/Interfaces/Image.h"

class ImageImpl final : public IImage
{
	Q_OBJECT

	struct VirtualTable {
		void(*setPixel)(ImageImpl * image, int x, int y, const vec4 & c);
		vec4(*getPixel)(const ImageImpl * image, int x, int y);

		vec3(*getPixelColor)(const ImageImpl * image, int x, int y);
		float(*getPixelOpacity)(const ImageImpl * image, int x, int y);
	};

	static const VirtualTable g_vtableGrayscaleB;
	static const VirtualTable g_vtableRgbB;
	static const VirtualTable g_vtableRgbaB;
	static const VirtualTable g_vtableGrayscaleF;
	static const VirtualTable g_vtableRgbF;
	static const VirtualTable g_vtableRgbaF;

	static void setPixelGrayscaleB(ImageImpl * image, int x, int y, const vec4 & c);
	static vec4 getPixelGrayscaleB(const ImageImpl * image, int x, int y);
	static vec3 getPixelColorGrayscaleB(const ImageImpl * image, int x, int y);
	static float getPixelOpacityGrayscaleB(const ImageImpl * image, int x, int y);

	static void setPixelRgbB(ImageImpl * image, int x, int y, const vec4 & c);
	static vec4 getPixelRgbB(const ImageImpl * image, int x, int y);
	static vec3 getPixelColorRgbB(const ImageImpl * image, int x, int y);
	static float getPixelOpacityRgbB(const ImageImpl * image, int x, int y);

	static void setPixelRgbaB(ImageImpl * image, int x, int y, const vec4 & c);
	static vec4 getPixelRgbaB(const ImageImpl * image, int x, int y);
	static vec3 getPixelColorRgbaB(const ImageImpl * image, int x, int y);
	static float getPixelOpacityRgbaB(const ImageImpl * image, int x, int y);

	static void setPixelGrayscaleF(ImageImpl * image, int x, int y, const vec4 & c);
	static vec4 getPixelGrayscaleF(const ImageImpl * image, int x, int y);
	static vec3 getPixelColorGrayscaleF(const ImageImpl * image, int x, int y);
	static float getPixelOpacityGrayscaleF(const ImageImpl * image, int x, int y);

	static void setPixelRgbF(ImageImpl * image, int x, int y, const vec4 & c);
	static vec4 getPixelRgbF(const ImageImpl * image, int x, int y);
	static vec3 getPixelColorRgbF(const ImageImpl * image, int x, int y);
	static float getPixelOpacityRgbF(const ImageImpl * image, int x, int y);

	static void setPixelRgbaF(ImageImpl * image, int x, int y, const vec4 & c);
	static vec4 getPixelRgbaF(const ImageImpl * image, int x, int y);
	static vec3 getPixelColorRgbaF(const ImageImpl * image, int x, int y);
	static float getPixelOpacityRgbaF(const ImageImpl * image, int x, int y);

	template <typename Func>
	void processRowsParallel(int height, Func func) const;

	QString        m_name;
	int            m_width;
	int            m_height;
	eImageDataType   m_dataType;
	eImageFormat     m_format;
	eImageColorSpace m_colorSpace = eImageColorSpace::Unknown;
	vec2             m_dpi;
	std::vector<byte>    m_data;
	const VirtualTable * m_vtable;

	void allocate(int w, int h, eImageFormat fmt, eImageDataType dataType, eImageColorSpace colorSpace, const vec2 & dpi);

	inline byte *dataB() { return m_data.data(); }
	inline const byte *dataB() const { return m_data.data(); }

	inline float *dataF() { return (float *)m_data.data(); }
	inline const float *dataF() const { return (float *)m_data.data(); }

public:
	ImageImpl(int w, int h, eImageFormat fmt, eImageDataType dataType, eImageColorSpace colorSpace, const vec2 & dpi = vec2(72.f));
	ImageImpl(int w, int h, eImageFormat fmt, eImageColorSpace colorSpace, const byte * src, const vec2 & dpi = vec2(72.f));
	ImageImpl(int w, int h, eImageFormat fmt, eImageColorSpace colorSpace, const float * src, const vec2 & dpi = vec2(72.f));
	virtual ~ImageImpl();

	int width() const final override { return m_width; }
	int height() const final override { return m_height; }

	void setName(const QString & name) final override { m_name = name; }
	const QString & name() const final override { return m_name; }

	eImageFormat format() const final override { return m_format; }
	eImageDataType dataType() const final override { return m_dataType; }
	eImageColorSpace colorSpace() const final override { return m_colorSpace; }
	void setColorSpace(eImageColorSpace colorSpace) { m_colorSpace = colorSpace; }
	const vec2 & dpi() const override { return m_dpi; }
	const void * rawData() const final override { return m_data.data(); }
	inline void * rawData() { return m_data.data(); }

	int channelCount() const
	{
		static const int s_channelCounts[] = { 1, 3, 4 };
		return s_channelCounts[static_cast<int>(m_format)];
	}

	int channelSize() const
	{
		static const int s_channelSizes[] = { 1, 4 };
		return s_channelSizes[static_cast<int>(m_dataType)];
	}

	void setPixel(int x, int y, const vec4 & c) final override
	{
		m_vtable->setPixel(this, x, y, c);
	}

	vec4 getPixel(int x, int y) const final override
	{
		return m_vtable->getPixel(this, x, y);
	}

	vec3 getPixelColor(int x, int y) const final override
	{
		return m_vtable->getPixelColor(this, x, y);
	}

	float getPixelOpacity(int x, int y) const final override
	{
		return m_vtable->getPixelOpacity(this, x, y);
	}

	vec4 getPixelUV(float u, float v, bool clampU, bool clampV) const final override;
	vec3 getPixelColorUV(float u, float v, bool clampU, bool clampV) const final override;
	float getPixelOpacityUV(float u, float v, bool clampU, bool clampV) const final override;

	void copyFrom(const ImagePtr & src, eAlphaProcessing processing = eAlphaProcessing::None) override;
	QImage toQImage(bool convertPremultipliedAlphaToStraight = false) const override;

	void swap(ImageImpl & otherImage);
};
