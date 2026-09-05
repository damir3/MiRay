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

class IObjectsFactory;
class ILog;
class ISettings;

#include "../../Shared/Interfaces/Image.h"

class ImageManager final : public IImageManager
{
	Q_OBJECT

	ImageSavingFormats m_savingFormats;
	ImageLoadingFormats m_loadingFormats;

	FileFormatsList m_loadingFilters;
	FileFormatsList m_savingFilters;

	std::map<QString, std::array<std::weak_ptr<IImage>, 3>> m_loadedImages;
	mutable QMutex m_mutex; // m_loadedImages locker
	IObjectsFactory & m_factory;
	const ILog & m_log;

	ImagePtr _loadImage(const QString &fileName, bool fallback, eImageColorSpace cs) const;
	ImagePtr _createImage(struct FIBITMAP * & bitmap, int fiFormat, eImageColorSpace cs) const;

	struct BitmapInfo;
	static void _normalizeLdrBitmap(struct FIBITMAP * & bitmap, BitmapInfo & info);
	void _convertPixelData(struct FIBITMAP * bitmap, const BitmapInfo & info, void * imageRawData) const;
	static void _processAlpha(struct FIBITMAP * bitmap, int fiFormat, const BitmapInfo & info, void * imageRawData);

	void _reloadImage(const QString & path);

public:
	ImageManager(IObjectsFactory &f, const ILog &log);
	~ImageManager();

	const ImageSavingFormats & savingFormats() const override { return m_savingFormats; }
	const ImageLoadingFormats & loadingFormats() const override { return m_loadingFormats; }

	const FileFormatsList & loadingFilters() const override { return m_loadingFilters; }
	const FileFormatsList & savingFilters() const override { return m_savingFilters; }

	ImagePtr createImage(int width, int height, eImageFormat format, eImageDataType type, eImageColorSpace colorSpace, const vec2 & dpi = vec2(72.f)) const override;
	ImagePtr createImage(int width, int height, eImageFormat format, eImageDataType type, eImageColorSpace colorSpace, const void *data, const vec2 & dpi = vec2(72.f)) const override;
	ImagePtr createImage(const QImage &image, eImageFormat format, eImageDataType type, eImageColorSpace colorSpace) const override;

	bool isImageFile(const QString &path) const override;
	ImagePtr loadImage(const QByteArray & data, const QString & extension, bool fallback, eImageColorSpace cs = eImageColorSpace::Unknown) const override;
	ImagePtr loadImage(const QString & path, bool fallback, eImageColorSpace cs = eImageColorSpace::Unknown) override;
	QByteArray saveImage(ImagePtr image, const QString & formatExtension, float quality = 1.f) const override;

	QStringList getLoadedImages() const;
	void reloadImages() override;

	ImagePtr convertImage(ImagePtr image, eImageFormat format, eImageDataType type, eImageColorSpace colorSpace, eAlphaProcessing processing = eAlphaProcessing::None) const override;
	ImagePtr scaleImage(ImagePtr image, int width, int height, eImageFormat format, eImageDataType type, eScaleFilter filter) const override;

	QString getPreviewBase64(const QString & path, int maxSize, int minSize) override;
};
