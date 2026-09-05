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

#include "ImageImpl.h"
#include "Interfaces/Image.h"

// ------------------------------------------------------------------------ //

const ImageImpl::VirtualTable ImageImpl::g_vtableGrayscaleB = {
	&ImageImpl::setPixelGrayscaleB,
	&ImageImpl::getPixelGrayscaleB,
	&ImageImpl::getPixelColorGrayscaleB,
	&ImageImpl::getPixelOpacityGrayscaleB,
};

const ImageImpl::VirtualTable ImageImpl::g_vtableRgbB = {
	&ImageImpl::setPixelRgbB,
	&ImageImpl::getPixelRgbB,
	&ImageImpl::getPixelColorRgbB,
	&ImageImpl::getPixelOpacityRgbB,
};

const ImageImpl::VirtualTable ImageImpl::g_vtableRgbaB = {
	&ImageImpl::setPixelRgbaB,
	&ImageImpl::getPixelRgbaB,
	&ImageImpl::getPixelColorRgbaB,
	&ImageImpl::getPixelOpacityRgbaB,
};


const ImageImpl::VirtualTable ImageImpl::g_vtableGrayscaleF = {
	&ImageImpl::setPixelGrayscaleF,
	&ImageImpl::getPixelGrayscaleF,
	&ImageImpl::getPixelColorGrayscaleF,
	&ImageImpl::getPixelOpacityGrayscaleF,
};

const ImageImpl::VirtualTable ImageImpl::g_vtableRgbF = {
	&ImageImpl::setPixelRgbF,
	&ImageImpl::getPixelRgbF,
	&ImageImpl::getPixelColorRgbF,
	&ImageImpl::getPixelOpacityRgbF,
};

const ImageImpl::VirtualTable ImageImpl::g_vtableRgbaF = {
	&ImageImpl::setPixelRgbaF,
	&ImageImpl::getPixelRgbaF,
	&ImageImpl::getPixelColorRgbaF,
	&ImageImpl::getPixelOpacityRgbaF,
};

static float sRGB_grayscale(const glm::vec4 & c)
{
	return 0.2989f * c.r + 0.5870f * c.g + 0.1140f * c.b;
}

static float RGB_grayscale(const glm::vec4 & c)
{
	return 0.212671f * c.r + 0.715160f * c.g + 0.072169f * c.b;
}

// ------------------------------------------------------------------------ //

ImageImpl::ImageImpl(int width, int height, eImageFormat format, eImageDataType dataType, eImageColorSpace colorSpace, const vec2 & dpi)
{
	allocate(width, height, format, dataType, colorSpace, dpi);
}

ImageImpl::ImageImpl(int width, int height, eImageFormat format, eImageColorSpace colorSpace, const byte * src, const vec2 & dpi)
{
	allocate(width, height, format, eImageDataType::Byte, colorSpace, dpi);
	std::memcpy(dataB(), src, static_cast<size_t>(width) * height * channelCount());
}

ImageImpl::ImageImpl(int width, int height, eImageFormat format, eImageColorSpace colorSpace, const float * src, const vec2 & dpi)
{
	allocate(width, height, format, eImageDataType::Float, colorSpace, dpi);
	std::memcpy(dataF(), src, static_cast<size_t>(width) * height * channelCount() * sizeof(float));
}

ImageImpl::~ImageImpl()
{
}

void ImageImpl::allocate(int width, int height, eImageFormat format, eImageDataType dataType, eImageColorSpace colorSpace, const vec2 & dpi)
{
	m_vtable = nullptr;
	if (dataType == eImageDataType::Byte) {
		switch (format) {
			case eImageFormat::Grayscale: m_vtable = &g_vtableGrayscaleB; break;
			case eImageFormat::RGB:       m_vtable = &g_vtableRgbB; break;
			case eImageFormat::RGBA:      m_vtable = &g_vtableRgbaB; break;
		}
	} else if (dataType == eImageDataType::Float) {
		switch (format) {
			case eImageFormat::Grayscale: m_vtable = &g_vtableGrayscaleF; break;
			case eImageFormat::RGB:       m_vtable = &g_vtableRgbF; break;
			case eImageFormat::RGBA:      m_vtable = &g_vtableRgbaF; break;
		}
	}
	assert(m_vtable);

	m_width = width;
	m_height = height;
	m_format = format;
	m_dataType = dataType;
	m_colorSpace = colorSpace;
	m_dpi = dpi;

	int c = channelCount();
	size_t numElements = width * height * c;

	try {
		m_data.resize(numElements * channelSize());
	} catch (const std::bad_alloc &) {
		throw std::runtime_error(QString("Unable to allocate %1x%2x%3 %4 of memory").arg(width).arg(height).arg(c).arg(dataType == eImageDataType::Byte ? "bytes" : "floats").toStdString());
	}

	switch (dataType) {
		case eImageDataType::Byte:
			std::fill(dataB(), dataB() + numElements, static_cast<byte>(255));
			break;
		case eImageDataType::Float:
			std::fill(dataF(), dataF() + numElements, 1.f);
			break;
	}
}

vec4 ImageImpl::getPixelUV(float u, float v, bool clampU, bool clampV) const
{
	assert(isfinite(u) && isfinite(v));
	if (!isfinite(u) || !isfinite(v))
		return vec4(0.f, 0.f, 0.f, 0.f);

	assert(u >= 0.f && u <= 1.f);
	assert(v >= 0.f && v <= 1.f);

	auto fx = u * m_width - 0.5f;
	auto fy = (1.f - v) * m_height - 0.5f;

	auto ix = (int)std::floor(fx);
	auto iy = (int)std::floor(fy);
	fx -= ix;
	fy -= iy;
	if (ix < 0) ix = clampU ? 0 : (m_width - 1);
	if (iy < 0) iy = clampV ? 0 : (m_height - 1);
	auto ix2 = ix + 1;
	auto iy2 = iy + 1;
	if (ix2 >= m_width)  ix2 = clampU ? ix : 0;
	if (iy2 >= m_height) iy2 = clampV ? iy : 0;

	auto c11 = getPixel(ix, iy);
	auto c12 = getPixel(ix2, iy);
	auto c21 = getPixel(ix, iy2);
	auto c22 = getPixel(ix2, iy2);
	auto c1 = glm::mix(c11, c12, fx);
	auto c2 = glm::mix(c21, c22, fx);

	return glm::mix(c1, c2, fy);
}

vec3 ImageImpl::getPixelColorUV(float u, float v, bool clampU, bool clampV) const
{
	assert(isfinite(u) && isfinite(v));
	if (!isfinite(u) || !isfinite(v))
		return vec3(0.f);

	assert(u >= 0.f && u <= 1.f);
	assert(v >= 0.f && v <= 1.f);

	auto fx = u * m_width - 0.5f;
	auto fy = (1.f - v) * m_height - 0.5f;

	auto ix = (int)std::floor(fx);
	auto iy = (int)std::floor(fy);
	fx -= ix;
	fy -= iy;
	if (ix < 0) ix = clampU ? 0 : (m_width - 1);
	if (iy < 0) iy = clampV ? 0 : (m_height - 1);
	auto ix2 = ix + 1;
	auto iy2 = iy + 1;
	if (ix2 >= m_width)  ix2 = clampU ? ix : 0;
	if (iy2 >= m_height) iy2 = clampV ? iy : 0;

	auto c11 = getPixelColor(ix, iy);
	auto c12 = getPixelColor(ix2, iy);
	auto c21 = getPixelColor(ix, iy2);
	auto c22 = getPixelColor(ix2, iy2);
	auto c1 = glm::mix(c11, c12, fx);
	auto c2 = glm::mix(c21, c22, fx);

	return glm::mix(c1, c2, fy);
}

float ImageImpl::getPixelOpacityUV(float u, float v, bool clampU, bool clampV) const
{
	assert(isfinite(u) && isfinite(v));
	if (!isfinite(u) || !isfinite(v))
		return 0.f;

	assert(u >= 0.f && u <= 1.f);
	assert(v >= 0.f && v <= 1.f);

	auto fx = u * m_width - 0.5f;
	auto fy = (1.f - v) * m_height - 0.5f;

	auto ix = (int)std::floor(fx);
	auto iy = (int)std::floor(fy);
	fx -= ix;
	fy -= iy;
	if (ix < 0) ix = clampU ? 0 : (m_width - 1);
	if (iy < 0) iy = clampV ? 0 : (m_height - 1);
	auto ix2 = ix + 1;
	auto iy2 = iy + 1;
	if (ix2 >= m_width)  ix2 = clampU ? ix : 0;
	if (iy2 >= m_height) iy2 = clampV ? iy : 0;

	auto c11 = getPixelOpacity(ix, iy);
	auto c12 = getPixelOpacity(ix2, iy);
	auto c21 = getPixelOpacity(ix, iy2);
	auto c22 = getPixelOpacity(ix2, iy2);
	auto c1 = lerp<float>(c11, c12, fx);
	auto c2 = lerp<float>(c21, c22, fx);

	return lerp<float>(c1, c2, fy);
}

// ------------------------------------------------------------------------ //

void ImageImpl::setPixelGrayscaleB(ImageImpl * image, int x, int y, const vec4 & c)
{
	assert(x >= 0 && x < image->m_width);
	assert(y >= 0 && y < image->m_height);

	image->dataB()[y * image->m_width + x] = SF2B(sRGB_grayscale(c));
}

vec4 ImageImpl::getPixelGrayscaleB(const ImageImpl * image, int x, int y)
{
	assert(x >= 0 && x < image->m_width);
	assert(y >= 0 && y < image->m_height);

	float c = B2F(image->dataB()[y * image->m_width + x]);
	return vec4(c, c, c, 1.f);
}

vec3 ImageImpl::getPixelColorGrayscaleB(const ImageImpl * image, int x, int y)
{
	assert(x >= 0 && x < image->m_width);
	assert(y >= 0 && y < image->m_height);

	return vec3(B2F(image->dataB()[y * image->m_width + x]));
}

float ImageImpl::getPixelOpacityGrayscaleB(const ImageImpl * image, int x, int y)
{
	return 1.f;
}

// ------------------------------------------------------------------------ //

void ImageImpl::setPixelRgbB(ImageImpl * image, int x, int y, const vec4 & c)
{
	assert(x >= 0 && x < image->m_width);
	assert(y >= 0 && y < image->m_height);

	auto * p = image->dataB() + 3 * (y * image->m_width + x);
	p[0] = SF2B(c.r);
	p[1] = SF2B(c.g);
	p[2] = SF2B(c.b);
}

vec4 ImageImpl::getPixelRgbB(const ImageImpl * image, int x, int y)
{
	assert(x >= 0 && x < image->m_width);
	assert(y >= 0 && y < image->m_height);

	const auto * p = image->dataB() + 3 * (y * image->m_width + x);
	return vec4(B2F(p[0]), B2F(p[1]), B2F(p[2]), 1.f);
}

vec3 ImageImpl::getPixelColorRgbB(const ImageImpl * image, int x, int y)
{
	assert(x >= 0 && x < image->m_width);
	assert(y >= 0 && y < image->m_height);

	const auto * p = image->dataB() + 3 * (y * image->m_width + x);
	return vec3(B2F(p[0]), B2F(p[1]), B2F(p[2]));
}

float ImageImpl::getPixelOpacityRgbB(const ImageImpl * image, int x, int y)
{
	return 1.f;
}

// ------------------------------------------------------------------------ //

void ImageImpl::setPixelRgbaB(ImageImpl * image, int x, int y, const vec4 & c)
{
	assert(x >= 0 && x < image->m_width);
	assert(y >= 0 && y < image->m_height);

	auto * p = image->dataB() + 4 * (y * image->m_width + x);
	p[0] = SF2B(c.r);
	p[1] = SF2B(c.g);
	p[2] = SF2B(c.b);
	p[3] = SF2B(c.a);
}

vec4 ImageImpl::getPixelRgbaB(const ImageImpl * image, int x, int y)
{
	assert(x >= 0 && x < image->m_width);
	assert(y >= 0 && y < image->m_height);

	const auto * p = image->dataB() + 4 * (y * image->m_width + x);
	return vec4(B2F(p[0]), B2F(p[1]), B2F(p[2]), B2F(p[3]));
}

vec3 ImageImpl::getPixelColorRgbaB(const ImageImpl * image, int x, int y)
{
	assert(x >= 0 && x < image->m_width);
	assert(y >= 0 && y < image->m_height);

	const auto * p = image->dataB() + 4 * (y * image->m_width + x);
	return vec3(B2F(p[0]), B2F(p[1]), B2F(p[2]));
}

float ImageImpl::getPixelOpacityRgbaB(const ImageImpl * image, int x, int y)
{
	assert(x >= 0 && x < image->m_width);
	assert(y >= 0 && y < image->m_height);

	const auto * p = image->dataB() + 4 * (y * image->m_width + x);
	return B2F(p[3]);
}

// ------------------------------------------------------------------------ //

void ImageImpl::setPixelGrayscaleF(ImageImpl * image, int x, int y, const vec4 & c)
{
	assert(x >= 0 && x < image->m_width);
	assert(y >= 0 && y < image->m_height);

	image->dataF()[y * image->m_width + x] = RGB_grayscale(c);
}

vec4 ImageImpl::getPixelGrayscaleF(const ImageImpl * image, int x, int y)
{
	assert(x >= 0 && x < image->m_width);
	assert(y >= 0 && y < image->m_height);

	float c = image->dataF()[y * image->m_width + x];
	return vec4(c, c, c, 1.f);
}

vec3 ImageImpl::getPixelColorGrayscaleF(const ImageImpl * image, int x, int y)
{
	assert(x >= 0 && x < image->m_width);
	assert(y >= 0 && y < image->m_height);

	return vec3(image->dataF()[y * image->m_width + x]);
}

float ImageImpl::getPixelOpacityGrayscaleF(const ImageImpl * image, int x, int y)
{
	return 1.f;
}

// ------------------------------------------------------------------------ //

void ImageImpl::setPixelRgbF(ImageImpl * image, int x, int y, const vec4 & c)
{
	assert(x >= 0 && x < image->m_width);
	assert(y >= 0 && y < image->m_height);

	auto * p = image->dataF() + 3 * (y * image->m_width + x);
	p[0] = c.r;
	p[1] = c.g;
	p[2] = c.b;
}

vec4 ImageImpl::getPixelRgbF(const ImageImpl * image, int x, int y)
{
	assert(x >= 0 && x < image->m_width);
	assert(y >= 0 && y < image->m_height);

	const auto * p = image->dataF() + 3 * (y * image->m_width + x);
	return vec4(p[0], p[1], p[2], 1.f);
}

vec3 ImageImpl::getPixelColorRgbF(const ImageImpl * image, int x, int y)
{
	assert(x >= 0 && x < image->m_width);
	assert(y >= 0 && y < image->m_height);

	const auto * p = image->dataF() + 3 * (y * image->m_width + x);
	return vec3(p[0], p[1], p[2]);
}

float ImageImpl::getPixelOpacityRgbF(const ImageImpl * image, int x, int y)
{
	return 1.f;
}

// ------------------------------------------------------------------------ //

void ImageImpl::setPixelRgbaF(ImageImpl * image, int x, int y, const vec4 & c)
{
	assert(x >= 0 && x < image->m_width);
	assert(y >= 0 && y < image->m_height);

	auto * p = image->dataF() + 4 * (y * image->m_width + x);
	p[0] = c.r;
	p[1] = c.g;
	p[2] = c.b;
	p[3] = c.a;
}

vec4 ImageImpl::getPixelRgbaF(const ImageImpl * image, int x, int y)
{
	assert(x >= 0 && x < image->m_width);
	assert(y >= 0 && y < image->m_height);

	const auto * p = image->dataF() + 4 * (y * image->m_width + x);
	return vec4(p[0], p[1], p[2], p[3]);
}

vec3 ImageImpl::getPixelColorRgbaF(const ImageImpl * image, int x, int y)
{
	assert(x >= 0 && x < image->m_width);
	assert(y >= 0 && y < image->m_height);

	const auto * p = image->dataF() + 4 * (y * image->m_width + x);
	return vec3(p[0], p[1], p[2]);
}

float ImageImpl::getPixelOpacityRgbaF(const ImageImpl * image, int x, int y)
{
	assert(x >= 0 && x < image->m_width);
	assert(y >= 0 && y < image->m_height);

	const auto * p = image->dataF() + 4 * (y * image->m_width + x);
	return p[3];
}

// ------------------------------------------------------------------------ //

template <typename Func>
void ImageImpl::processRowsParallel(int height, Func func) const
{
	const unsigned int numThreads = std::max(1u, std::thread::hardware_concurrency());
	const int rowsPerThread = (height + static_cast<int>(numThreads) - 1) / static_cast<int>(numThreads);

	std::vector<std::future<void>> futures;
	futures.reserve(numThreads);

	for (unsigned int i = 0; i < numThreads; ++i) {
		const int fromY = static_cast<int>(i) * rowsPerThread;
		const int toY = std::min(fromY + rowsPerThread, height);
		if (fromY >= toY)
			break;

		futures.push_back(std::async(std::launch::async, func, fromY, toY));
	}

	for (auto& f : futures)
		f.get();
}

void ImageImpl::copyFrom(const ImagePtr & src, eAlphaProcessing processing)
{
	if (!src) {
		throw std::invalid_argument("Invalid parameter 'src' passed to method 'Image::copyFrom()': src is null");
	}
	if (src->width() != m_width || src->height() != m_height) {
		throw std::invalid_argument("Invalid parameter 'src' passed to method 'Image::copyFrom()': Invalid source image size: "
									+ std::to_string(src->width()) + "x" + std::to_string(src->height())
									+ " (must be " + std::to_string(m_width) + "x" + std::to_string(m_height) + ")");
	}

	const bool srgbToLinear = src->colorSpace() == eImageColorSpace::sRGB  && m_colorSpace == eImageColorSpace::Linear;
	const bool linearToSRGB = src->colorSpace() == eImageColorSpace::Linear && m_colorSpace == eImageColorSpace::sRGB;

	const int width = m_width;
	const int height = m_height;

	processRowsParallel(height, [=, this](int fromY, int toY) {
		for (int y = fromY; y < toY; ++y) {
			for (int x = 0; x < width; ++x) {
				auto c = src->getPixel(x, y);

				if (processing == eAlphaProcessing::StraightToPremultiplied) {
					if (linearToSRGB) {
						c = ColorUtils::linearToSRGB(c);
					} else if (srgbToLinear) {
						c = ColorUtils::sRGBToLinear(c);
					}

					c = ColorUtils::straightToPremultiplied(c);
				} else {
					if (processing == eAlphaProcessing::PremultipliedToStraight)
						c = ColorUtils::premultipliedToStraight(c);

					if (linearToSRGB) {
						c = ColorUtils::linearToSRGB(c);
					} else if (srgbToLinear) {
						c = ColorUtils::sRGBToLinear(c);
					}
				}

				setPixel(x, y, c);
			}
		}
	});
}

// ------------------------------------------------------------------------ //

QImage ImageImpl::toQImage(bool convertPremultipliedAlphaToStraight) const
{
	QImage::Format format;
	switch (m_format) {
		case eImageFormat::Grayscale:
			format = QImage::Format_Grayscale8;
			convertPremultipliedAlphaToStraight = false;
			break;
		case eImageFormat::RGB:
			format = QImage::Format_RGB888;
			convertPremultipliedAlphaToStraight = false;
			break;
		case eImageFormat::RGBA:
			format = QImage::Format_ARGB32;
			break;
		default:
			assert(false);
			return QImage();
	}

	QImage res(m_width, m_height, format);
	res.bits();

	const int width = m_width;
	const int height = m_height;
	const bool convertColorLinearToSRGB = m_colorSpace == eImageColorSpace::Linear;

	processRowsParallel(height, [=, this, &res](int fromY, int toY) {
		for (int y = fromY; y < toY; ++y) {
			for (int x = 0; x < width; ++x) {
				auto clr = getPixel(x, y);

				if (convertPremultipliedAlphaToStraight)
					clr = ColorUtils::premultipliedToStraight(clr);

				if (convertColorLinearToSRGB)
					clr = ColorUtils::linearToSRGB(clr);

				res.setPixel(x, y, qRgba(SF2B(clr.x), SF2B(clr.y), SF2B(clr.z), SF2B(clr.w)));
			}
		}
	});

	return res;
}

// ------------------------------------------------------------------------ //

void ImageImpl::swap(ImageImpl & otherImage)
{
	std::swap(m_vtable, otherImage.m_vtable);
	std::swap(m_width, otherImage.m_width);
	std::swap(m_height, otherImage.m_height);
	std::swap(m_format, otherImage.m_format);
	std::swap(m_dataType, otherImage.m_dataType);
	std::swap(m_colorSpace, otherImage.m_colorSpace);
	std::swap(m_data, otherImage.m_data);
	std::swap(m_name, otherImage.m_name);
}
