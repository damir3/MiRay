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

#include "ImageManager.h"
#include "ImageImpl.h"
#include "ColorUtils.h"

#include <FreeImage.h>
#include <skcms.h>

#include "../../Shared/Interfaces/ObjectsFactory.h"
#include "../../Shared/Interfaces/Log.h"
#include "../../Shared/Interfaces/ApplicationContext.h"

ImageManager::ImageManager(IObjectsFactory &f, const ILog &log)
	: m_factory(f)
	, m_log(log)
{
	FreeImage_Initialise(true);
	FreeImage_SetOutputMessage([](FREE_IMAGE_FORMAT fif, const char *message) {
		Q_UNUSED(fif);
		// m_log.error(QString("FreeImage: %1").arg(message));
		qDebug() << "FreeImage:" << message;
	});

	// initialize loading formats supported by FreeImage
	m_loadingFormats = {
		{ "image/png", "PNG files", { "png" } },
		{ "image/jpeg", "JPG files", { "jpg", "jpeg" } },
		{ "image/x-exr", "EXR files", { "exr" } },
		{ "image/vnd.radiance", "HDR files", { "hdr" } },
		{ "image/tiff", "TIFF files", { "tif", "tiff" } },
		{ "image/bmp", "BMP files", { "bmp" } },
		{ "image/vnd.adobe.photoshop", "PSD files", { "psd" } },
		{ "image/x-portable-floatmap", "PFM files", { "pfm" } },
	};

	// initialize saving formats supported by FreeImage
	m_savingFormats = {
		{ "image/png", "PNG files", "png", eImageDataType::Byte, true },
		{ "image/jpeg", "JPG files", "jpg", eImageDataType::Byte, false },
		{ "image/x-exr", "EXR files", "exr", eImageDataType::Float, true },
		{ "image/vnd.radiance", "HDR files", "hdr", eImageDataType::Float, true },
		{ "image/tiff", "TIFF files", "tiff", eImageDataType::Byte, true },
		{ "image/bmp", "BMP files", "bmp", eImageDataType::Byte, true },
		{ "image/vnd.adobe.photoshop", "PSD files", "psd", eImageDataType::Byte, false },
	};

	// populate loading filters from loading formats
	for (const auto &fmt : m_loadingFormats)
		m_loadingFilters.push_back({fmt.fileType, fmt.extensions});

	// populate saving filters from saving formats
	for (const auto &fmt : m_savingFormats)
		m_savingFilters.push_back({fmt.fileType, {fmt.extension}});
}

ImageManager::~ImageManager()
{
	FreeImage_DeInitialise();
}

// ------------------------------------------------------------------------ //

ImagePtr ImageManager::createImage(int width, int height, eImageFormat format, eImageDataType dataType, eImageColorSpace colorSpace, const vec2 & dpi) const
{
	assert(dataType == eImageDataType::Byte || dataType == eImageDataType::Float);
	return std::make_shared<ImageImpl>(width, height, format, dataType, colorSpace, dpi);
}

ImagePtr ImageManager::createImage(int width, int height, eImageFormat format, eImageDataType dataType, eImageColorSpace colorSpace, const void *data, const vec2 & dpi) const
{
	assert(dataType == eImageDataType::Byte || dataType == eImageDataType::Float);
	return dataType == eImageDataType::Byte ?
			std::make_shared<ImageImpl>(width, height, format, colorSpace, static_cast<const byte *>(data), dpi) :
			std::make_shared<ImageImpl>(width, height, format, colorSpace, static_cast<const float *>(data), dpi);
}

ImagePtr ImageManager::createImage(const QImage &image, eImageFormat format, eImageDataType dataType, eImageColorSpace colorSpace) const
{
	auto dest = createImage(image.width(), image.height(), format, dataType, colorSpace);

	for (int y = 0; y < image.height(); y++) {
		for (int x = 0; x < image.width(); x++) {
			auto color = image.pixelColor(x, y);
			dest->setPixel(x, y, vec4(color.redF(), color.greenF(), color.blueF(), color.alphaF()));
		}
	}

	return dest;
}

// ------------------------------------------------------------------------ //

static double getResolutionFromExif(FIBITMAP *bitmap, const char *tagName)
{
	FITAG *tag = nullptr;
	if (!FreeImage_GetMetadata(FIMD_EXIF_MAIN, bitmap, tagName, &tag) || !tag)
		return 0.0;

	double resolution = 0.0;
	const void *value = FreeImage_GetTagValue(tag);
	if (!value)
		return 0.0;

	switch (FreeImage_GetTagType(tag)) {
		case FIDT_RATIONAL: {
			const auto *rational = static_cast<const uint32_t *>(value);
			if (rational[1] != 0) {
				resolution = static_cast<double>(rational[0]) / rational[1];
			}
			break;
		}
		case FIDT_SRATIONAL: {
			const auto *srational = static_cast<const int32_t *>(value);
			if (srational[1] != 0) {
				resolution = static_cast<double>(srational[0]) / srational[1];
			}
			break;
		}
		case FIDT_LONG:
			resolution = static_cast<double>(*static_cast<const uint32_t *>(value));
			break;
		case FIDT_SHORT:
			resolution = static_cast<double>(*static_cast<const uint16_t *>(value));
			break;
		case FIDT_FLOAT:
			resolution = static_cast<double>(*static_cast<const float *>(value));
			break;
		case FIDT_DOUBLE:
			resolution = *static_cast<const double *>(value);
			break;
		default:
			break;
	}

	if (resolution <= 0.0 || !std::isfinite(resolution))
		return 0.0;

	FITAG *unitTag = nullptr;
	if (FreeImage_GetMetadata(FIMD_EXIF_MAIN, bitmap, "ResolutionUnit", &unitTag) && unitTag) {
		const void *unitVal = FreeImage_GetTagValue(unitTag);
		if (unitVal) {
			uint16_t unit = 0;
			if (FreeImage_GetTagType(unitTag) == FIDT_SHORT) {
				unit = *static_cast<const uint16_t *>(unitVal);
			} else if (FreeImage_GetTagType(unitTag) == FIDT_LONG) {
				unit = static_cast<uint16_t>(*static_cast<const uint32_t *>(unitVal));
			}

			if (unit == 3)
				resolution *= 2.54; // dots/cm to dots/inch
		}
	}

	return resolution;
}

static double dpiX(FIBITMAP *bitmap)
{
	double dpi = getResolutionFromExif(bitmap, "XResolution");
	if (dpi > 0.0)
		return dpi;

	const unsigned dpm = FreeImage_GetDotsPerMeterX(bitmap);
	if (dpm > 0) {
		dpi = dpm * 0.0254;
		const double rounded = std::round(dpi);
		if (std::abs(dpi - rounded) < 0.02) {
			dpi = rounded;
		}
		return dpi;
	}

	return 0.0;
}

static double dpiY(FIBITMAP *bitmap)
{
	double dpi = getResolutionFromExif(bitmap, "YResolution");
	if (dpi > 0.0)
		return dpi;

	const unsigned dpm = FreeImage_GetDotsPerMeterY(bitmap);
	if (dpm > 0) {
		dpi = dpm * 0.0254;
		const double rounded = std::round(dpi);
		if (std::abs(dpi - rounded) < 0.02) {
			dpi = rounded;
		}
		return dpi;
	}

	return 0.0;
}

// ------------------------------------------------------------------------ //

enum class eColorType {
	Unknown,
	Gray,
	RGB,
	CMYK
};

static skcms_ICCProfile getLinearColorProfile()
{
	skcms_ICCProfile profile = *skcms_sRGB_profile();
	skcms_TransferFunction linearFn = { 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	skcms_SetTransferFunction(&profile, &linearFn);
	return profile;
}

static skcms_ICCProfile getDefaultProfile(eColorType profile);

static skcms_ICCProfile getLinearGrayProfile()
{
	skcms_ICCProfile profile = getDefaultProfile(eColorType::Gray);
	skcms_TransferFunction linearFn = { 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	skcms_SetTransferFunction(&profile, &linearFn);
	if (!profile.has_toXYZD50) {
		profile.data_color_space = skcms_Signature_Gray;
		profile.toXYZD50.vals[0][0] = 0.96420f;
		profile.toXYZD50.vals[1][1] = 1.00000f;
		profile.toXYZD50.vals[2][2] = 0.82491f;
		profile.has_toXYZD50 = true;
	}
	return profile;
}

static skcms_ICCProfile getDefaultProfile(eColorType profile)
{
	switch (profile) {
		case eColorType::Gray: {
			QByteArray grayProfile = readFile(":/sgray.icm");
			skcms_ICCProfile icc;
			if (skcms_Parse(grayProfile.data(), grayProfile.size(), &icc)) {
				return icc;
			}
			skcms_ICCProfile fallback = *skcms_sRGB_profile();
			fallback.data_color_space = skcms_Signature_Gray;
			fallback.toXYZD50.vals[0][0] = 0.96420f;
			fallback.toXYZD50.vals[1][1] = 1.00000f;
			fallback.toXYZD50.vals[2][2] = 0.82491f;
			fallback.has_toXYZD50 = true;
			return fallback;
		}
		case eColorType::RGB:
			return *skcms_sRGB_profile();
		case eColorType::CMYK: {
			QByteArray cmykProfile = readFile(":/rswop.icm");
			skcms_ICCProfile icc;
			if (skcms_Parse(cmykProfile.data(), cmykProfile.size(), &icc)) {
				return icc;
			}
			break;
		}
		default:
			break;
	}
	return *skcms_sRGB_profile();
}

static skcms_ICCProfile getLinearProfile(eColorType profile)
{
	return profile == eColorType::Gray ? getLinearGrayProfile() : getLinearColorProfile();
}

static bool convertRGBPaletteToSRGB(RGBQUAD *rgba, int size, const skcms_ICCProfile &profileIn)
{
	return skcms_Transform(rgba, skcms_PixelFormat_BGRA_8888, skcms_AlphaFormat_Unpremul, &profileIn,
							rgba, skcms_PixelFormat_BGRA_8888, skcms_AlphaFormat_Unpremul, skcms_sRGB_profile(),
							size);
}

static bool convertGrayscalePaletteToSRGB(RGBQUAD *rgba, int size, const skcms_ICCProfile &profileIn)
{
	std::vector<uint8_t> palette(size);
	for (int i = 0; i < size; i++) {
		palette[i] = rgba[i].rgbRed;
	}
	return skcms_Transform(palette.data(), skcms_PixelFormat_G_8, skcms_AlphaFormat_Opaque, &profileIn,
							rgba, skcms_PixelFormat_BGRA_8888, skcms_AlphaFormat_Opaque, skcms_sRGB_profile(),
							size);
}

static bool convertPaletteToSRGB(RGBQUAD *rgba, int size, FIICCPROFILE *colorProfile)
{
	if (!colorProfile || !colorProfile->data || colorProfile->size == 0) {
		return false;
	}
	skcms_ICCProfile profileIn;
	if (!skcms_Parse(colorProfile->data, colorProfile->size, &profileIn)) {
		return false;
	}

	if (profileIn.data_color_space == skcms_Signature_RGB) {
		return convertRGBPaletteToSRGB(rgba, size, profileIn);
	}
	if (profileIn.data_color_space == skcms_Signature_Gray) {
		return convertGrayscalePaletteToSRGB(rgba, size, profileIn);
	}

	return false;
}

// ------------------------------------------------------------------------ //

bool ImageManager::isImageFile(const QString & path) const
{
	auto ext = QFileInfo(path).suffix();
	for (const auto &fmt : m_loadingFormats) {
		for (const auto &e : fmt.extensions)
			if (e.compare(ext, Qt::CaseInsensitive) == 0)
				return true;
	}

	return false;
}

ImagePtr ImageManager::loadImage(const QByteArray & data, const QString & extension, bool fallback, eImageColorSpace colorSpace) const
{
	ImagePtr image;

	if (FIMEMORY *hmem = FreeImage_OpenMemory((byte *)data.data(), data.length())) {
		const auto format = FreeImage_GetFileTypeFromMemory(hmem, 0);
		if (format != FIF_UNKNOWN) {
			int flags = 0;
			switch (format) {
				case FIF_JPEG: flags = JPEG_CMYK | JPEG_ACCURATE; break;
				case FIF_TIFF: flags = TIFF_CMYK; break;
				case FIF_PSD:  flags = PSD_CMYK | PSD_LAB; break;
			}

			if (auto * bitmap = FreeImage_LoadFromMemory(format, hmem, flags)) {
				const int width = FreeImage_GetWidth(bitmap);
				const int height = FreeImage_GetHeight(bitmap);

				try {
					image = _createImage(bitmap, format, colorSpace);
				} catch (const std::bad_alloc &) {
					m_log.error(QString("Can't allocate memory for %1x%2 image").arg(width).arg(height));
				} catch (const std::exception & exception) {
					m_log.error(exception.what());
				}

				FreeImage_Unload(bitmap);
			} else {
				m_log.error(QString("Can't load image from memory."));
			}
		} else {
			m_log.error(QString("Unknown image format."));
		}

		FreeImage_CloseMemory(hmem);
	}

	if (!image.get() && fallback) { // make a dummy 1x1 image
		image = createImage(1, 1, eImageFormat::RGBA, eImageDataType::Byte, colorSpace);
	}

	return image;
}

struct ImageManager::BitmapInfo {
	int width = 0;
	int height = 0;
	int bpp = 0;
	bool isHDR = false;
	bool outputFloat = false;

	eImageFormat imageFormat = eImageFormat::RGB;
	eImageDataType dataType = eImageDataType::Byte;
	eImageColorSpace actualColorSpace = eImageColorSpace::Unknown;

	eColorType colorTypeIn = eColorType::Unknown;
	eColorType colorTypeOut = eColorType::Unknown;
	skcms_ICCProfile inProfile {};
	skcms_ICCProfile outProfile {};
	skcms_PixelFormat srcFormat {};
	skcms_PixelFormat dstFormat {};
};

// normalize LDR bitmaps into directly-usable pixel layouts
void ImageManager::_normalizeLdrBitmap(FIBITMAP * & bitmap, BitmapInfo & info)
{
	auto colorType = FreeImage_GetColorType(bitmap);
	auto colorProfile = FreeImage_GetICCProfile(bitmap);

	// decide whether we need FreeImage_ConvertTo32Bits
	bool needsConversion = false;

	if (colorType == FIC_PALETTE) {
		needsConversion = true;
	} else if (info.bpp == 8 && colorType == FIC_MINISBLACK) {
		needsConversion = false; // simple grayscale is fine
	} else if (info.bpp != 24 && info.bpp != 32) {
		needsConversion = true; // unusual bit depth (1, 4, 16, 48, 64 bpp)
	} else {
		auto * pal = FreeImage_GetPalette(bitmap);
		auto palSize = FreeImage_GetColorsUsed(bitmap);
		bool hasICCProfile = colorProfile && colorProfile->data && colorProfile->size > 0;
		if (pal && palSize > 0 && !hasICCProfile)
			needsConversion = true; // has palette data but no ICC - let FreeImage expand it
	}

	if (needsConversion) {
		// if there's a palette with an ICC profile, convert palette colors to sRGB first,
		// because FreeImage_ConvertTo32Bits will apply the palette but not the ICC
		auto * pal = FreeImage_GetPalette(bitmap);
		auto palSize = FreeImage_GetColorsUsed(bitmap);
		if (pal && palSize > 0 && colorProfile && colorProfile->data && colorProfile->size > 0) {
			convertPaletteToSRGB(pal, palSize, colorProfile);
			colorProfile = nullptr;
		}

		auto * converted = FreeImage_ConvertTo32Bits(bitmap);
		if (!converted)
			throw std::runtime_error("Failed to convert image to 32-bit (bpp=" + std::to_string(info.bpp) +
			                         ", colorType=" + std::to_string(colorType) + ")");

		FreeImage_Unload(bitmap);
		bitmap = converted;
		info.bpp = FreeImage_GetBPP(bitmap);
		colorType = FreeImage_GetColorType(bitmap);
		colorProfile = FreeImage_GetICCProfile(bitmap);
	}

	// handle 8-bit grayscale with transparency table: treat as 32bpp for channel count
	const bool transparentGray = (info.bpp == 8 && FreeImage_IsTransparent(bitmap));
	const int effectiveBpp = transparentGray ? 32 : info.bpp;

	// map FreeImage color type + effective bpp to skcms source pixel format
	if (colorType == FIC_CMYK) {
		info.colorTypeIn = (effectiveBpp == 8) ? eColorType::Gray : eColorType::CMYK;
		info.srcFormat = (effectiveBpp == 8) ? skcms_PixelFormat_G_8 : skcms_PixelFormat_RGBA_8888;
	} else if (effectiveBpp == 8) {
		info.colorTypeIn = eColorType::Gray;
		info.srcFormat = skcms_PixelFormat_G_8;
	} else if (effectiveBpp == 24) {
		info.colorTypeIn = eColorType::RGB;
		info.srcFormat = skcms_PixelFormat_BGR_888; // FreeImage stores LDR RGB as BGR
	} else if (effectiveBpp == 32) {
		info.colorTypeIn = eColorType::RGB;
		info.srcFormat = skcms_PixelFormat_BGRA_8888;
	} else {
		throw std::runtime_error("Unsupported pixel layout: bpp=" + std::to_string(effectiveBpp) +
		                         ", colorType=" + std::to_string(colorType));
	}

	// determine output image format from effective bpp
	if (effectiveBpp == 8 && !transparentGray)
		info.imageFormat = eImageFormat::Grayscale;
	else if (effectiveBpp == 32)
		info.imageFormat = eImageFormat::RGBA;
	else
		info.imageFormat = eImageFormat::RGB;
}

// color-convert scanlines from FIBITMAP into destination buffer
void ImageManager::_convertPixelData(FIBITMAP * bitmap, const BitmapInfo & info, void * imageRawData) const
{
	const auto srcType = FreeImage_GetImageType(bitmap);
	if (srcType == FIT_FLOAT) {
		const bool toSRGB = (info.actualColorSpace == eImageColorSpace::sRGB);
		if (info.outputFloat) {
			for (int y = 0; y < info.height; y++) {
				const float * srcLine = reinterpret_cast<const float *>(FreeImage_GetScanLine(bitmap, info.height - y - 1));
				float * dst = static_cast<float *>(imageRawData) + info.width * y;
				if (toSRGB) {
					for (int x = 0; x < info.width; x++)
						dst[x] = ColorUtils::linearToSRGB(srcLine[x]);
				} else {
					std::memcpy(dst, srcLine, info.width * sizeof(float));
				}
			}
		} else {
			for (int y = 0; y < info.height; y++) {
				const float * srcLine = reinterpret_cast<const float *>(FreeImage_GetScanLine(bitmap, info.height - y - 1));
				byte * dst = static_cast<byte *>(imageRawData) + info.width * y;
				if (toSRGB) {
					for (int x = 0; x < info.width; x++)
						dst[x] = SF2B(ColorUtils::linearToSRGB(srcLine[x]));
				} else {
					for (int x = 0; x < info.width; x++)
						dst[x] = SF2B(srcLine[x]);
				}
			}
		}
		return;
	}

	const int channelCount = (info.imageFormat == eImageFormat::Grayscale) ? 1 :
	                         (info.imageFormat == eImageFormat::RGB) ? 3 : 4;

	skcms_AlphaFormat srcAlpha = (channelCount == 4 || info.colorTypeIn == eColorType::CMYK)
	                             ? skcms_AlphaFormat_Unpremul : skcms_AlphaFormat_Opaque;
	skcms_AlphaFormat dstAlpha = (channelCount == 4)
	                             ? skcms_AlphaFormat_Unpremul : skcms_AlphaFormat_Opaque;

	// for HDR source with non-float output, skcms can't directly convert float->byte,
	// so we use an intermediate float buffer and quantize manually
	const bool needsIntermediateFloat = info.isHDR && !info.outputFloat;

	if (needsIntermediateFloat) {
		const int intermediateChannels = (channelCount == 1) ? 3 : channelCount;
		skcms_PixelFormat intermediateFmt = (intermediateChannels <= 3)
		                                    ? skcms_PixelFormat_RGB_fff : skcms_PixelFormat_RGBA_ffff;
		std::vector<float> floatLine(info.width * intermediateChannels);

		for (int y = 0; y < info.height; y++) {
			const void * srcLine = FreeImage_GetScanLine(bitmap, info.height - y - 1);
			skcms_Transform(srcLine, info.srcFormat, srcAlpha, &info.inProfile,
			                floatLine.data(), intermediateFmt, dstAlpha, &info.outProfile, info.width);

			if (channelCount == 1) {
				byte * dst = static_cast<byte *>(imageRawData) + info.width * y;
				for (int x = 0; x < info.width; x++)
					dst[x] = SF2B(floatLine[x * 3]);
			} else {
				byte * dst = static_cast<byte *>(imageRawData) + info.width * y * channelCount;
				for (int x = 0; x < info.width * channelCount; x++)
					dst[x] = SF2B(floatLine[x]);
			}
		}
	} else if (channelCount == 1) {
		// grayscale: for float output, skcms outputs RGB_fff - extract first channel; for byte output, skcms supports G_8 directly
		if (info.outputFloat) {
			std::vector<float> rgbLine(info.width * 3);
			for (int y = 0; y < info.height; y++) {
				const void * srcLine = FreeImage_GetScanLine(bitmap, info.height - y - 1);
				float * dst = static_cast<float *>(imageRawData) + info.width * y;
				if (skcms_Transform(srcLine, info.srcFormat, srcAlpha, &info.inProfile,
				                    rgbLine.data(), skcms_PixelFormat_RGB_fff, dstAlpha, &info.outProfile, info.width)) {
					for (int x = 0; x < info.width; x++)
						dst[x] = rgbLine[x * 3];
				} else {
					const byte * src = static_cast<const byte *>(srcLine);
					for (int x = 0; x < info.width; x++)
						dst[x] = B2F(src[x]);
				}
			}
		} else {
			for (int y = 0; y < info.height; y++) {
				const void * srcLine = FreeImage_GetScanLine(bitmap, info.height - y - 1);
				byte * dst = static_cast<byte *>(imageRawData) + info.width * y;
				if (!skcms_Transform(srcLine, info.srcFormat, srcAlpha, &info.inProfile,
				                     dst, skcms_PixelFormat_G_8, dstAlpha, &info.outProfile, info.width)) {
					memcpy(dst, srcLine, info.width);
				}
			}
		}
	} else {
		// rgb / rgba: direct per-scanline conversion
		const int bytesPerPixel = info.outputFloat ? (channelCount * (int)sizeof(float)) : channelCount;
		const skcms_ICCProfile defaultIn = getDefaultProfile(info.colorTypeIn);
		const skcms_ICCProfile defaultOut = getDefaultProfile(info.colorTypeOut);
		for (int y = 0; y < info.height; y++) {
			const void * srcLine = FreeImage_GetScanLine(bitmap, info.height - y - 1);
			byte * dst = static_cast<byte *>(imageRawData) + info.width * y * bytesPerPixel;
			if (!skcms_Transform(srcLine, info.srcFormat, srcAlpha, &info.inProfile,
			                     dst, info.dstFormat, dstAlpha, &info.outProfile, info.width)) {
				skcms_Transform(srcLine, info.srcFormat, srcAlpha, &defaultIn,
				                dst, info.dstFormat, dstAlpha, &defaultOut, info.width);
			}
		}
	}
}

// extract alpha from source bitmap and convert premultiplied ↔ straight
void ImageManager::_processAlpha(FIBITMAP * bitmap, int fiFormat, const BitmapInfo & info, void * imageRawData)
{
	if (info.imageFormat != eImageFormat::RGBA)
		return;

	const int pixelCount = info.width * info.height;
	bool srcIsPremultiplied = false;

	// helpers to reduce repetition

	// iterate scanlines with Y-flip, calling fn(src, dst) per pixel.
	// TSrc/TDst are source/destination element types; srcStride is the source advance per pixel.
	auto forEachScanline = [&](int srcStride, auto fn) {
		for (int y = 0; y < info.height; y++) {
			const byte * scanLine = FreeImage_GetScanLine(bitmap, info.height - y - 1);
			if (info.outputFloat) {
				float * dst = static_cast<float *>(imageRawData) + y * info.width * 4;
				for (int x = 0; x < info.width; x++, dst += 4, scanLine += srcStride)
					fn(scanLine, dst);
			} else {
				byte * dst = static_cast<byte *>(imageRawData) + y * info.width * 4;
				for (int x = 0; x < info.width; x++, dst += 4, scanLine += srcStride)
					fn(scanLine, dst);
			}
		}
	};

	// iterate every pixel in flat layout (stride 4), calling fn(d + i) per pixel.
	auto forEachPixel4 = [&](auto fn) {
		if (info.outputFloat) {
			float * d = static_cast<float *>(imageRawData);
			for (int i = 0; i < pixelCount * 4; i += 4) fn(d + i);
		} else {
			byte * d = static_cast<byte *>(imageRawData);
			for (int i = 0; i < pixelCount * 4; i += 4) fn(d + i);
		}
	};

	// fill alpha channel with opaque value (1.0f or 255).
	auto fillOpaqueAlpha = [&]() {
		forEachPixel4([](auto * p) { p[3] = std::is_same_v<std::remove_pointer_t<decltype(p)>, float> ? 1.f : 255; });
	};

	// extract alpha channel from the source bitmap
	if (FreeImage_IsTransparent(bitmap)) {
		const auto srcBpp = FreeImage_GetBPP(bitmap);

		if (srcBpp == 8) {
			// 8-bit grayscale with transparency table
			srcIsPremultiplied = true;
			unsigned numItems = FreeImage_GetTransparencyCount(bitmap);
			const byte * table = FreeImage_GetTransparencyTable(bitmap);

			forEachScanline(1, [&](const byte * src, auto * dst) {
				if constexpr (std::is_same_v<std::remove_pointer_t<decltype(dst)>, float>)
					dst[3] = (*src < numItems) ? B2F(table[*src]) : 1.f;
				else
					dst[3] = (*src < numItems) ? table[*src] : 255;
			});
		} else if (srcBpp == 32) {
			// 32bpp LDR - check TIFF premultiplied alpha flag
			if (fiFormat == FIF_TIFF) {
				FITAG * tag = nullptr;
				if (FreeImage_GetMetadata(FIMD_EXIF_MAIN, bitmap, "ExtraSamples", &tag) &&
					FreeImage_GetTagType(tag) == FIDT_SHORT &&
					FreeImage_GetTagLength(tag) >= 2) {
					auto tagData = (const unsigned short *)FreeImage_GetTagValue(tag);
					srcIsPremultiplied = tagData[0] == 1; // EXTRASAMPLE_ASSOCALPHA
				}
			}

			// for byte output, alpha was already placed by skcms (BGRA->RGBA)
			if (info.outputFloat) {
				forEachScanline(4, [](const byte * src, auto * dst) {
					if constexpr (std::is_same_v<std::remove_pointer_t<decltype(dst)>, float>)
						dst[3] = B2F(src[3]);
				});
			}
		} else if (srcBpp == 128) {
			// 128bpp HDR float - EXR stores premultiplied alpha
			srcIsPremultiplied = (fiFormat == FIF_EXR);

			forEachScanline(16, [](const byte * src, auto * dst) {
				const float * srcF = reinterpret_cast<const float *>(src);
				if constexpr (std::is_same_v<std::remove_pointer_t<decltype(dst)>, float>)
					dst[3] = srcF[3];
				else
					dst[3] = SF2B(srcF[3]);
			});
		} else {
			// unknown source bpp - fill alpha with opaque
			fillOpaqueAlpha();
		}
	} else {
		// source has no transparency - fill alpha with opaque
		fillOpaqueAlpha();
	}

	// convert between premultiplied ↔ straight alpha
	if (info.actualColorSpace == eImageColorSpace::Linear && !srcIsPremultiplied) {
		// linear wants premultiplied - source is straight -> multiply RGB by A
		if (info.outputFloat) {
			float * d = static_cast<float *>(imageRawData);
			for (int i = 0; i < pixelCount * 4; i += 4) {
				float a = d[i + 3];
				if (a < 1.f) { d[i] *= a; d[i + 1] *= a; d[i + 2] *= a; }
			}
		}
	} else if (info.actualColorSpace != eImageColorSpace::Linear && srcIsPremultiplied) {
		// sRGB/Unknown wants straight - source is premultiplied -> divide RGB by A
		forEachPixel4([](auto * p) {
			if constexpr (std::is_same_v<std::remove_pointer_t<decltype(p)>, float>) {
				float a = p[3];
				if (a > 0.f && a != 1.f) {
					float f = 1.f / a;
					p[0] *= f; p[1] *= f; p[2] *= f;
				}
			} else {
				float a = p[3] * (1.f / 255.f);
				if (a > 0.f && a < 1.f) {
					float f = 1.f / a;
					p[0] = SF2B(B2F(p[0]) * f);
					p[1] = SF2B(B2F(p[1]) * f);
					p[2] = SF2B(B2F(p[2]) * f);
				}
			}
		});
	}
}

ImagePtr ImageManager::_createImage(FIBITMAP * & bitmap, int fiFormat, eImageColorSpace colorSpace) const
{
	BitmapInfo info;
	info.width = FreeImage_GetWidth(bitmap);
	info.height = FreeImage_GetHeight(bitmap);
	info.bpp = FreeImage_GetBPP(bitmap);

	const auto type = FreeImage_GetImageType(bitmap);
	const vec2 dpi(dpiX(bitmap), dpiY(bitmap));

	info.isHDR = (type == FIT_RGBF || type == FIT_RGBAF || type == FIT_FLOAT);

	// determine target dataType and actualColorSpace
	switch (colorSpace) {
		case eImageColorSpace::Linear:
			info.dataType = eImageDataType::Float;
			info.actualColorSpace = eImageColorSpace::Linear;
			break;
		case eImageColorSpace::sRGB:
			info.dataType = eImageDataType::Byte;
			info.actualColorSpace = eImageColorSpace::sRGB;
			break;
		case eImageColorSpace::Unknown:
		default:
			info.dataType = info.isHDR ? eImageDataType::Float : eImageDataType::Byte;
			info.actualColorSpace = info.isHDR ? eImageColorSpace::Linear : eImageColorSpace::sRGB;
			break;
	}
	info.outputFloat = (info.dataType == eImageDataType::Float);

	// analyze source bitmap and determine pixel formats
	if (info.isHDR) {
		if (type == FIT_FLOAT) {
			if (info.bpp != 32)
				throw std::runtime_error("Unsupported HDR bits per pixel: " + std::to_string(info.bpp));

			info.colorTypeIn = eColorType::Gray;
			info.srcFormat = skcms_PixelFormat_RGB_fff;
			info.imageFormat = eImageFormat::Grayscale;
		} else {
			if (info.bpp != 96 && info.bpp != 128)
				throw std::runtime_error("Unsupported HDR bits per pixel: " + std::to_string(info.bpp));

			const bool hasAlpha = (type == FIT_RGBAF);
			info.colorTypeIn = eColorType::RGB;
			info.srcFormat = hasAlpha ? skcms_PixelFormat_RGBA_ffff : skcms_PixelFormat_RGB_fff;
			info.imageFormat = hasAlpha ? eImageFormat::RGBA : eImageFormat::RGB;
		}
	} else {
		_normalizeLdrBitmap(bitmap, info);
	}

	// determine output color type
	switch (info.imageFormat) {
		case eImageFormat::Grayscale: info.colorTypeOut = eColorType::Gray; break;
		case eImageFormat::RGB:       info.colorTypeOut = eColorType::RGB;  break;
		case eImageFormat::RGBA:      info.colorTypeOut = eColorType::RGB;  break;
	}

	// set up ICC profiles for color conversion
	auto colorProfile = FreeImage_GetICCProfile(bitmap);
	bool hasProfileIn = false;

	if (colorSpace == eImageColorSpace::Unknown) {
		// identity transform - preserve pixel values as-is
		if (info.isHDR) {
			info.inProfile = getLinearColorProfile();
			info.outProfile = getLinearColorProfile();
		} else {
			info.inProfile = getLinearProfile(info.colorTypeIn);
			info.outProfile = getLinearProfile(info.colorTypeOut);
		}
		hasProfileIn = true;
	}

	if (!hasProfileIn) {
		if (colorProfile && colorProfile->data && colorProfile->size > 0)
			hasProfileIn = skcms_Parse(colorProfile->data, colorProfile->size, &info.inProfile);
		if (!hasProfileIn)
			info.inProfile = info.isHDR ? getLinearColorProfile() : getDefaultProfile(info.colorTypeIn);
	}

	if (colorSpace != eImageColorSpace::Unknown) {
		info.outProfile = (info.actualColorSpace == eImageColorSpace::Linear)
		                  ? getLinearProfile(info.colorTypeOut)
		                  : getDefaultProfile(info.colorTypeOut);
	}

	// determine destination pixel format
	const int channelCount = (info.imageFormat == eImageFormat::Grayscale) ? 1 :
	                         (info.imageFormat == eImageFormat::RGB) ? 3 : 4;

	if (channelCount == 1)
		info.dstFormat = info.outputFloat ? skcms_PixelFormat_RGB_fff : skcms_PixelFormat_G_8;
	else if (channelCount == 3)
		info.dstFormat = info.outputFloat ? skcms_PixelFormat_RGB_fff : skcms_PixelFormat_RGB_888;
	else
		info.dstFormat = info.outputFloat ? skcms_PixelFormat_RGBA_ffff : skcms_PixelFormat_RGBA_8888;

	// create output image and run conversions
	auto image = createImage(info.width, info.height, info.imageFormat, info.dataType, info.actualColorSpace, dpi);
	void * imageRawData = static_cast<ImageImpl *>(image.get())->rawData();

	_convertPixelData(bitmap, info, imageRawData);
	_processAlpha(bitmap, fiFormat, info, imageRawData);

	return image;
}

ImagePtr ImageManager::_loadImage(const QString &fileName, bool fallback, eImageColorSpace colorSpace) const
{
	try {
		m_log.information(QString("Loading image: %1").arg(fileName));

		const auto appContext = qobject_cast<IApplicationContext *>(qApp);
		assert(appContext);

		const auto data = readFile(appContext->getFullResourcePath(fileName));
		return loadImage(data, QFileInfo(fileName).suffix(), fallback, colorSpace);
	} catch (const std::exception &ex) {
		m_log.warning(QString("Error loading image '%1': %2").arg(fileName).arg(ex.what()));
	}

	if (fallback)
		return createImage(1, 1, eImageFormat::RGBA, eImageDataType::Byte, eImageColorSpace::Unknown, vec2(1, 1));

	return nullptr;
}

ImagePtr ImageManager::loadImage(const QString & path, bool fallback, eImageColorSpace colorSpace)
{
	assert(static_cast<size_t>(colorSpace) < 3);

	QMutexLocker locker(&m_mutex);

	const auto appContext = qobject_cast<IApplicationContext *>(qApp);
	const auto normalizedPath = appContext ? appContext->getShortResourcePath(path) : path;

	QString realPath;
	if (normalizedPath.startsWith("miray://") || normalizedPath.startsWith("owlet://")) {
		realPath = normalizedPath;
	} else {
		QFileInfo fi(normalizedPath);
		if (fi.exists())
			realPath = fi.canonicalFilePath();
		realPath = nativePath(realPath.isEmpty() ? normalizedPath : realPath);
	}

	auto it = m_loadedImages.find(realPath);
	if (it != m_loadedImages.end()) {
		if (auto img = it->second[(size_t)colorSpace].lock())
			return img;
	}

	auto img = _loadImage(realPath, fallback, colorSpace);
	if (img) {
		m_loadedImages[realPath][(size_t)colorSpace] = img;
	}

	return img;
}

QByteArray ImageManager::saveImage(ImagePtr srcImage, const QString & formatExtension, float quality) const
{
	if (!srcImage) {
		m_log.error("SaveImage: Null image pointer passed.");
		return QByteArray();
	}

	FREE_IMAGE_FORMAT fif = FreeImage_GetFIFFromFilename(("dummy." + formatExtension).toUtf8().constData());
	if (fif == FIF_UNKNOWN) {
		m_log.error(QString("SaveImage: Unknown format extension: %1").arg(formatExtension));
		return QByteArray();
	}

	if (!FreeImage_FIFSupportsWriting(fif)) {
		m_log.error(QString("SaveImage: FreeImage does not support writing format for extension: %1").arg(formatExtension));
		return QByteArray();
	}

	const auto width = srcImage->width();
	const auto height = srcImage->height();
	const auto format = fif == FIF_HDR ? eImageFormat::RGB : srcImage->format();
	const auto dataType = fif == FIF_HDR || fif == FIF_EXR ? eImageDataType::Float : eImageDataType::Byte;
	const auto colorSpace = dataType == eImageDataType::Float ? eImageColorSpace::Linear : eImageColorSpace::sRGB;
//	qDebug() << srcImage->name() << srcImage->width() << height << (int)srcImage->format() << (int)srcImage->dataType() << formatExtension;
	if (srcImage->format() != format || srcImage->dataType() != dataType || srcImage->colorSpace() != colorSpace) {
		srcImage = convertImage(srcImage, format, dataType, colorSpace, eAlphaProcessing::None);
	//	qDebug() << "*" << srcImage->name() << srcImage->width() << srcImage->height() << (int)srcImage->format() << (int)srcImage->dataType();
	}

	FIBITMAP *dib = nullptr;

	if (dataType == eImageDataType::Byte) {
		if (format == eImageFormat::Grayscale) {
			dib = FreeImage_AllocateT(FIT_BITMAP, width, height, 8);
			if (dib) {
				RGBQUAD *pal = FreeImage_GetPalette(dib);
				for (int i = 0; i < 256; i++) {
					pal[i].rgbRed = i;
					pal[i].rgbGreen = i;
					pal[i].rgbBlue = i;
				}
				const byte *src = reinterpret_cast<const byte*>(srcImage->rawData());
				for (int y = 0; y < height; y++) {
					byte *dest = FreeImage_GetScanLine(dib, height - 1 - y);
					std::memcpy(dest, src + y * width, width);
				}
			}
		} else if (format == eImageFormat::RGB) {
			dib = FreeImage_AllocateT(FIT_BITMAP, width, height, 24);
			if (dib) {
				const byte *src = reinterpret_cast<const byte*>(srcImage->rawData());
				for (int y = 0; y < height; y++) {
					byte *dest = FreeImage_GetScanLine(dib, height - 1 - y);
					const byte *srcRow = src + y * width * 3;
					for (int x = 0; x < width; x++) {
						dest[x * 3 + 0] = srcRow[x * 3 + 2]; // b
						dest[x * 3 + 1] = srcRow[x * 3 + 1]; // g
						dest[x * 3 + 2] = srcRow[x * 3 + 0]; // r
					}
				}
			}
		} else if (format == eImageFormat::RGBA) {
			dib = FreeImage_AllocateT(FIT_BITMAP, width, height, 32);
			if (dib) {
				const byte *src = reinterpret_cast<const byte*>(srcImage->rawData());
				for (int y = 0; y < height; y++) {
					byte *dest = FreeImage_GetScanLine(dib, height - 1 - y);
					const byte *srcRow = src + y * width * 4;
					for (int x = 0; x < width; x++) {
						dest[x * 4 + 0] = srcRow[x * 4 + 2]; // b
						dest[x * 4 + 1] = srcRow[x * 4 + 1]; // g
						dest[x * 4 + 2] = srcRow[x * 4 + 0]; // r
						dest[x * 4 + 3] = srcRow[x * 4 + 3]; // a
					}
				}
			}
		}
	} else if (dataType == eImageDataType::Float) {
		FREE_IMAGE_TYPE type = FIT_UNKNOWN;
		int bpp = 0;
		int channels = 0;
		if (format == eImageFormat::Grayscale) {
			type = FIT_FLOAT;
			bpp = 32;
			channels = 1;
		} else if (format == eImageFormat::RGB) {
			type = FIT_RGBF;
			bpp = 96;
			channels = 3;
		} else if (format == eImageFormat::RGBA) {
			type = FIT_RGBAF;
			bpp = 128;
			channels = 4;
		}
		if (type != FIT_UNKNOWN) {
			dib = FreeImage_AllocateT(type, width, height, bpp);
			if (dib) {
				const float *src = reinterpret_cast<const float*>(srcImage->rawData());
				for (int y = 0; y < height; y++) {
					byte *dest = FreeImage_GetScanLine(dib, height - 1 - y);
					std::memcpy(dest, src + y * width * channels, width * channels * sizeof(float));
				}
			}
		}
	}

	if (!dib) {
		m_log.error(QString("SaveImage: Failed to create FreeImage bitmap for extension: %1").arg(formatExtension));
		return QByteArray();
	}

	vec2 dpi = srcImage->dpi();
	if (dpi.x > 0 && dpi.y > 0) {
		FreeImage_SetDotsPerMeterX(dib, static_cast<unsigned>(dpi.x / 0.0254 + 0.5));
		FreeImage_SetDotsPerMeterY(dib, static_cast<unsigned>(dpi.y / 0.0254 + 0.5));
	}

	// format support conversion
	FREE_IMAGE_TYPE imageType = FreeImage_GetImageType(dib);
	int bpp = FreeImage_GetBPP(dib);
	if (fif == FIF_JPEG && bpp == 32) {
		FIBITMAP *temp = FreeImage_ConvertTo24Bits(dib);
		if (temp) {
			FreeImage_Unload(dib);
			dib = temp;
		}
	} else if (!FreeImage_FIFSupportsExportType(fif, imageType) || !FreeImage_FIFSupportsExportBPP(fif, bpp)) {
		FIBITMAP *temp = nullptr;
		if (FreeImage_FIFSupportsExportBPP(fif, 24)) {
			temp = FreeImage_ConvertTo24Bits(dib);
		} else if (FreeImage_FIFSupportsExportBPP(fif, 32)) {
			temp = FreeImage_ConvertTo32Bits(dib);
		} else if (FreeImage_FIFSupportsExportBPP(fif, 8)) {
			temp = FreeImage_ConvertTo8Bits(dib);
		}
		if (temp) {
			FreeImage_Unload(dib);
			dib = temp;
		}
	}

	int flags = 0;
	if (fif == FIF_JPEG) {
		flags = qBound(1, static_cast<int>(quality * 100.f), 100);
	} else if (fif == FIF_PNG) {
		if (quality <= 0.0f) {
			flags = 0x0100; // PNG_Z_NO_COMPRESSION
		} else {
			flags = qBound(1, static_cast<int>(quality * 9.f), 9);
		}
	}

	FIMEMORY *hmem = FreeImage_OpenMemory(nullptr, 0);
	if (!hmem) {
		m_log.error("SaveImage: Failed to open FreeImage memory stream.");
		FreeImage_Unload(dib);
		return QByteArray();
	}

	BOOL success = FreeImage_SaveToMemory(fif, dib, hmem, flags);
	FreeImage_Unload(dib);

	QByteArray ba;
	if (success) {
		BYTE *data = nullptr;
		DWORD size_in_bytes = 0;
		if (FreeImage_AcquireMemory(hmem, &data, &size_in_bytes)) {
			ba = QByteArray(reinterpret_cast<const char*>(data), size_in_bytes);
		} else {
			m_log.error("SaveImage: Failed to acquire FreeImage memory pointer.");
		}
	} else {
		m_log.error(QString("SaveImage: Failed to save to memory for format extension '%1'").arg(formatExtension));
	}

	FreeImage_CloseMemory(hmem);
	return ba;
}


// ------------------------------------------------------------------------ //

QStringList ImageManager::getLoadedImages() const
{
	QMutexLocker locker(&m_mutex);

	QStringList list;
	for (auto it = m_loadedImages.cbegin(); it != m_loadedImages.cend(); ++it) {
		for (const auto & img : it->second) {
			if (!img.expired()) {
				list.append(it->first);
				break;
			}
		}
	}

	return list;
}

void ImageManager::_reloadImage(const QString & path)
{
	std::array<bool, 3> needReload = {};

	{
		QMutexLocker locker(&m_mutex);
		auto it = m_loadedImages.find(path);
		if (it == m_loadedImages.end()) return; // nothing to do here
		for (size_t i = 0; i < it->second.size(); ++i) {
			needReload[i] = !it->second[i].expired();
		}
	}

	std::array<ImagePtr, 3> reloadedImages;
	for (size_t i = 0; i < needReload.size(); ++i) {
		if (needReload[i]) {
			reloadedImages[i] = _loadImage(path, true, static_cast<eImageColorSpace>(i));
		}
	}

	QMutexLocker locker(&m_mutex);
	auto it = m_loadedImages.find(path);
	assert (it != m_loadedImages.end());

	emit beforeImageUpdate(path);

	if (it != m_loadedImages.end()) {
		for (size_t i = 0; i < reloadedImages.size(); ++i) {
			if (reloadedImages[i]) {
				auto oldImage = it->second[i].lock();

				auto image1 = reinterpret_cast<ImageImpl *>(oldImage.get());
				auto image2 = reinterpret_cast<ImageImpl *>(reloadedImages[i].get());
				assert(image1);
				assert(image2);

				if (image1 && image2) {
					image1->swap(*image2);
					emit imageUpdated(image1);
				}
			}
		}
	}

	emit afterImageUpdate(path);
}

void ImageManager::reloadImages()
{
	const auto loadedImages = getLoadedImages();
	for (const auto & path : loadedImages)
		_reloadImage(path);
}

// ------------------------------------------------------------------------ //

QString ImageManager::getPreviewBase64(const QString & path, int maxSize, int minSize)
{
	const auto appContext = qobject_cast<IApplicationContext *>(qApp);
	assert(appContext);

	if (QFileInfo(appContext->getFullResourcePath(path)).exists()) {
		if (auto image = loadImage(path, false)) {
			const auto maxImageSize = std::max(image->width(), image->height());
			const auto newImageSize = std::clamp(maxImageSize, minSize, maxSize);
			if (auto scaledImage = scaleImage(image, image->width() * newImageSize / maxImageSize, image->height() * newImageSize / maxImageSize,
											  eImageFormat::RGBA, eImageDataType::Byte, eScaleFilter::Triangle)) {
				const auto pngData = saveImage(scaledImage, "png", 1.f);
				return "data:image/png;base64," + pngData.toBase64();
			}
		}
	}

	return QString();
}

// ------------------------------------------------------------------------ //

ImagePtr ImageManager::convertImage(ImagePtr srcImage, eImageFormat format, eImageDataType type, eImageColorSpace colorSpace, eAlphaProcessing proc) const
{
	if (colorSpace == eImageColorSpace::Unknown)
		colorSpace = srcImage->colorSpace();

	if (srcImage->format() == format && srcImage->dataType() == type && proc == eAlphaProcessing::None &&
		(srcImage->colorSpace() == colorSpace || srcImage->colorSpace() == eImageColorSpace::Unknown))
		return srcImage; // no need to convert this

	auto destImage = createImage(srcImage->width(), srcImage->height(), format, type, colorSpace, srcImage->dpi());
	destImage->copyFrom(srcImage, proc);
	return destImage;
}

// ------------------------------------------------------------------------ //

enum { MAX_CHANNELS = 4 };

struct FilterInfo {
	double(*filter)(double), support;
};

struct ContributionInfo {
	int		offset;
	double	weight;
};

static double box(double x)
{
	return ((x >= -0.5) && (x < 0.5)) ? 1.0 : 0.0;
}

static double blackman(double x)
{
	return (0.42 + 0.50*cos(M_PI*x) + 0.08*cos(2.0*M_PI*x));
}

static double catrom(double x)
{
	if (x < 0)
		x = -x;
	if (x < 1.0)
		return (0.5*(2.0 + x*x*(-5.0 + x*3.0)));
	if (x < 2.0)
		return (0.5*(4.0 + x*(-8.0 + x*(5.0 - x))));
	return (0.0);
}

static double cubic(double x)
{
	if (x < 0)
		x = -x;
	if (x < 1.0)
		return ((0.5*x*x*x) - x*x + (2.0 / 3.0));
	if (x < 2.0) {
		x = 2.0 - x;
		return ((1.0 / 6.0)*x*x*x);
	}
	return (0.0);
}

static double gaussian(double x)
{
	return (exp(-2.0*x*x)*sqrt(2.0 / M_PI));
}

static double hanning(double x)
{
	return (0.5 + 0.5*cos(M_PI*x));
}

static double hamming(double x)
{
	return (0.54 + 0.46*cos(M_PI*x));
}

static double hermite(double x)
{
	if (x < 0)
		x = -x;
	if (x < 1.0)
		return ((2.0*x - 3.0)*x*x + 1.0);
	return (0.0);
}

static double sinc(double x)
{
	x *= M_PI;
	if (x != 0.0)
		return (sin(x) / x);
	return (1.0);
}

static double lanczos(double x)
{
	if (x < 0)
		x = -x;
	if (x < 3.0)
		return (sinc(x)*sinc(x / 3.0));
	return (0.0);
}

static double mitchell(double x)
{
	double	b, c;

	b = 1.0 / 3.0;
	c = 1.0 / 3.0;
	if (x < 0) x = -x;
	if (x < 1.0) {
		x = ((12.0 - 9.0*b - 6.0*c)*(x*x*x)) + ((-18.0 + 12.0*b + 6.0*c)*x*x) + (6.0 - 2.0*b);
		return (x / 6.0);
	}
	if (x < 2.0) {
		x = ((-1.0*b - 6.0*c)*(x*x*x)) + ((6.0*b + 30.0*c)*x*x) + ((-12.0*b - 48.0*c)*x) + (8.0*b + 24.0*c);
		return (x / 6.0);
	}
	return (0.0);
}

static double quadratic(double x)
{
	if (x < 0)
		x = -x;
	if (x < 0.5)
		return (0.75 - x*x);
	if (x < 1.5) {
		x -= 1.5;
		return (0.5*x*x);
	}
	return (0.0);
}

static double triangle(double x)
{
	if (x < 0.0)
		x = -x;
	if (x < 1.0)
		return (1.0 - x);
	return (0.0);
}

static void horizontalFilter(const IImage *srcImage, IImage *destImage, double xFactor, const FilterInfo & filter_info, ContributionInfo *contributionInfo)
{
	const int	cpp[3] = { 1, 3, 4 };
	const int	numChannels = cpp[(int)srcImage->format()];
	const int	srcWidth = srcImage->width();
	const int	destWidth = destImage->width(), destHeight = destImage->height();
	const int	destRow = destWidth * numChannels, srcRow = srcWidth * numChannels;
	double		cWeight[MAX_CHANNELS];

	// apply filter to zoom horizontally from source to destination.
	double scaleFactor = std::max(1.0 / xFactor, 1.0);
	double support = std::max(scaleFactor * filter_info.support, 0.5);
	if (support <= 0.5) { // reduce to point sampling.
		support = 0.5;
		scaleFactor = 1.0;
	}
	support += 1.0e-7;

	for (int x = 0; x < destWidth; x++) {
		const double center = (double)(x + 0.5) / xFactor;
		const int start = std::max<int>((int)(center - support - 0.5), 0);
		const int end = std::min<int>((int)(center + support + 0.5), srcWidth);
		double density = 0.0;
		int n = 0;
		for (int i = start; i < end; i++) {
			contributionInfo[n].offset = i * numChannels;
			contributionInfo[n].weight = filter_info.filter(((double)i - center + 0.5) / scaleFactor) / scaleFactor;
			density += contributionInfo[n].weight;
			n++;
		}
		if ((density != 0.0) && (density != 1.0)) {
			for (int i = 0; i < n; i++) {
				contributionInfo[i].weight /= density;
			}
		}

		switch (srcImage->dataType()) {
			case eImageDataType::Byte: {
				uint8_t * dest = (uint8_t *)destImage->rawData() + (x * numChannels);
				const uint8_t * src = (const uint8_t *)srcImage->rawData();
				for (int y = 0; y < destHeight; y++) {
					std::fill(cWeight, cWeight + numChannels, 0.0);
					for (int i = 0; i < n; i++) {
						const uint8_t * p = src + contributionInfo[i].offset;
						for (int c = 0; c < numChannels; c++)
							cWeight[c] += p[c] * contributionInfo[i].weight;
					}
					for (int c = 0; c < numChannels; c++)
						dest[c] = (uint8_t)std::clamp(cWeight[c] + 0.5, 0.0, 255.0);
					dest += destRow;
					src += srcRow;
				}
			}
			break;
			case eImageDataType::Float: {
				float * dest = (float *)destImage->rawData() + (x * numChannels);
				const float * src = (float *)srcImage->rawData();
				for (int y = 0; y < destHeight; y++) {
					std::fill(cWeight, cWeight + numChannels, 0.0);
					for (int i = 0; i < n; i++) {
						const float * p = src + contributionInfo[i].offset;
						for (int c = 0; c < numChannels; c++)
							cWeight[c] += p[c] * contributionInfo[i].weight;
					}
					for (int c = 0; c < numChannels; c++)
						dest[c] = (float)cWeight[c];
					dest += destRow;
					src += srcRow;
				}
			}
			break;
		}
	}
}

static void verticalFilter(const IImage *srcImage, IImage *destImage, double yFactor, const FilterInfo & filter_info, ContributionInfo *contributionInfo)
{
	const int	cpp[3] = { 1, 3, 4 };
	const int	numChannels = cpp[(int)srcImage->format()];
	const int	srcWidth = srcImage->width(), srcHeight = srcImage->height();
	const int	destWidth = destImage->width(), destHeight = destImage->height();
	const int	destRow = destImage->width() * numChannels;
	double		cWeight[MAX_CHANNELS];

	// apply filter to zoom vertically from source to destination.
	double		scaleFactor = std::max(1.0 / yFactor, 1.0);
	double		support = std::max(scaleFactor * filter_info.support, 0.5);
	if (support <= 0.5) { // reduce to point sampling.
		support = 0.5;
		scaleFactor = 1.0;
	}
	support += 1.0e-7;

	for (int y = 0; y < destHeight; y++) {
		const double center = ((double)y + 0.5) / yFactor;
		const int start = std::max<int>((int)(center - support - 0.5), 0);
		const int end = std::min<int>((int)(center + support + 0.5), srcHeight);
		double density = 0.0;
		int n = 0;
		for (int i = start; i < end; i++) {
			contributionInfo[n].offset = i * srcWidth * numChannels;
			contributionInfo[n].weight = filter_info.filter(((double)i - center + 0.5) / scaleFactor) / scaleFactor;
			density += contributionInfo[n].weight;
			n++;
		}
		if ((density != 0.0) && (density != 1.0)) {
			for (int i = 0; i < n; i++)
				contributionInfo[i].weight /= density;
		}

		switch (srcImage->dataType()) {
			case eImageDataType::Byte: {
				uint8_t * dest = (uint8_t *)destImage->rawData() + y * destRow;
				const uint8_t * src = (uint8_t *)srcImage->rawData();
				for (int x = 0; x < destWidth; x++) {
					std::fill(cWeight, cWeight + numChannels, 0.0);
					for (int i = 0; i < n; i++) {
						const uint8_t * p = src + contributionInfo[i].offset;
						for (int c = 0; c < numChannels; c++)
							cWeight[c] += p[c] * contributionInfo[i].weight;
					}
					for (int c = 0; c < numChannels; c++)
						dest[c] = (uint8_t)std::clamp(cWeight[c] + 0.5, 0.0, 255.0);
					dest += numChannels;
					src += numChannels;
				}
			}
			break;
			case eImageDataType::Float: {
				float * dest = (float *)destImage->rawData() + y * destRow;
				const float * src = (float *)srcImage->rawData();
				for (int x = 0; x < destWidth; x++) {
					std::fill(cWeight, cWeight + numChannels, 0.0);
					for (int i = 0; i < n; i++) {
						const float * p = src + contributionInfo[i].offset;
						for (int c = 0; c < numChannels; c++)
							cWeight[c] += p[c] * contributionInfo[i].weight;
					}
					for (int c = 0; c < numChannels; c++)
						dest[c] = (float)cWeight[c];
					dest += numChannels;
					src += numChannels;
				}
			}
			break;
		}
	}
}

ImagePtr ImageManager::scaleImage(ImagePtr srcImage, int width, int height, eImageFormat format, eImageDataType type, eScaleFilter sf) const
{
	auto srcWidth = srcImage->width();
	auto srcHeight = srcImage->height();
	if (width == srcWidth && height == srcHeight) {
		if (srcImage->format() == format && srcImage->dataType() == type)
			return srcImage;

		return convertImage(srcImage, format, type, srcImage->colorSpace());
	}

	static const FilterInfo	filters[] = {
		{ box, 0.0 },
		{ box, 0.5 },
		{ triangle, 1.0 },
		{ hermite, 1.0 },
		{ hanning, 1.0 },
		{ hamming, 1.0 },
		{ blackman, 1.0 },
		{ gaussian, 1.25 },
		{ quadratic, 1.5 },
		{ cubic, 2.0 },
		{ catrom, 2.0 },
		{ mitchell, 2.0 },
		{ lanczos, 3.0 },
		{ sinc, 4.0 }
	};

	auto & filter = filters[(int)sf];

	auto zoomedImage = createImage(width, height, srcImage->format(), srcImage->dataType(), srcImage->colorSpace(), srcImage->dpi());

	// allocate filter info list.
	const double xFactor = (double)width / (double)srcWidth;
	const double yFactor = (double)height / (double)srcHeight;
	const double support = filter.support / std::min(std::min(xFactor, yFactor), 1.0);

	std::vector<ContributionInfo> contributionInfo((int)(support * 2 + 3));

	if (xFactor >= yFactor) {
		auto sourceImage = createImage(width, srcHeight, srcImage->format(), srcImage->dataType(), srcImage->colorSpace(), srcImage->dpi());
		horizontalFilter(srcImage.get(), sourceImage.get(), xFactor, filter, contributionInfo.data());
		verticalFilter(sourceImage.get(), zoomedImage.get(), yFactor, filter, contributionInfo.data());
	} else {
		auto sourceImage = createImage(srcWidth, height, srcImage->format(), srcImage->dataType(), srcImage->colorSpace(), srcImage->dpi());
		verticalFilter(srcImage.get(), sourceImage.get(), yFactor, filter, contributionInfo.data());
		horizontalFilter(sourceImage.get(), zoomedImage.get(), xFactor, filter, contributionInfo.data());
	}

	if (zoomedImage->format() == format && zoomedImage->dataType() == type)
		return zoomedImage;

	return convertImage(zoomedImage, format, type, srcImage->colorSpace());
}
