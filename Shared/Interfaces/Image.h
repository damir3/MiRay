#pragma once

#include "../Utils/FileUtils.h"

enum class eImageColorSpace {
	Unknown = 0,
	sRGB,
	Linear,
};

enum class eImageFormat {
	Grayscale = 0,
	RGB,
	RGBA
};

enum class eImageDataType {
	Byte = 0,
	Float
};

enum class eScaleFilter {
	Point = 0,
	Box,
	Triangle,
	Hermite,
	Hanning,
	Hamming,
	Blackman,
	Gaussian,
	Quadratic,
	Cubic,
	Catrom,
	Mitchell,
	Lanczos,
	Sinc
};

enum class eAlphaProcessing {
	None = 0,
	PremultipliedToStraight,
	StraightToPremultiplied,
};

class IImageManager;
using ImagePtr = std::shared_ptr<class IImage>;

class SHAREDLIB_EXPORT IImage : public QObject
{
	Q_OBJECT

public:
	virtual ~IImage() {}

	virtual int width() const = 0;
	virtual int height() const = 0;

	virtual const QString & name() const = 0;
	virtual void setName(const QString & name) = 0;

	virtual eImageFormat format() const = 0;
	virtual eImageDataType dataType() const = 0;
	virtual eImageColorSpace colorSpace() const = 0;
	virtual const glm::vec2 & dpi() const = 0;
	virtual const void * rawData() const = 0;

	virtual void setPixel(int x, int y, const glm::vec4 & c) = 0;
	virtual glm::vec4 getPixel(int x, int y) const = 0;
	virtual glm::vec3 getPixelColor(int x, int y) const = 0;
	virtual float getPixelOpacity(int x, int y) const = 0;

	virtual glm::vec4 getPixelUV(float u, float v, bool clampU, bool clampV) const = 0;
	virtual glm::vec3 getPixelColorUV(float u, float v, bool clampU, bool clampV) const = 0;
	virtual float getPixelOpacityUV(float u, float v, bool clampU, bool clampV) const = 0;

	virtual void copyFrom(const ImagePtr & src, eAlphaProcessing flags = eAlphaProcessing::None) = 0;
	virtual QImage toQImage(bool convertPremultipliedAlphaToStraight = false) const = 0;
};

// ------------------------------------------------------------------------ //
// saving

struct ImageSavingFormat {
	QString	mimeType;
	QString fileType;
	QString extension;
	eImageDataType imageDataType;
	bool wantsStraightAlpha; // true if alpha needs to be normal, false if it should be premultiplied
};

typedef std::vector<ImageSavingFormat>	ImageSavingFormats;

// ------------------------------------------------------------------------ //
// loading

struct ImageLoadingFormat {
	QString	mimeType;
	QString fileType;
	QStringList extensions;
};

typedef std::vector<ImageLoadingFormat>	ImageLoadingFormats;

// ------------------------------------------------------------------------ //
// image manager

class SHAREDLIB_EXPORT IImageManager : public QObject
{
	Q_OBJECT

protected:
	virtual ~IImageManager() {}

public:
	virtual const ImageSavingFormats & savingFormats() const = 0;
	virtual const ImageLoadingFormats & loadingFormats() const = 0;

	virtual const FileFormatsList & loadingFilters() const = 0;
	virtual const FileFormatsList & savingFilters() const = 0;

	virtual ImagePtr createImage(int w, int h, eImageFormat fmt, eImageDataType type, eImageColorSpace colorSpace, const glm::vec2 & dpi = glm::vec2(72.f)) const = 0;
	virtual ImagePtr createImage(int w, int h, eImageFormat fmt, eImageDataType type, eImageColorSpace colorSpace, const void *data, const glm::vec2 & dpi = glm::vec2(72.f)) const = 0;
	virtual ImagePtr createImage(const QImage &image, eImageFormat fmt, eImageDataType type, eImageColorSpace colorSpace) const = 0;

	virtual bool isImageFile(const QString &path) const = 0;
	virtual ImagePtr loadImage(const QByteArray &ba, const QString &extension, bool fallback, eImageColorSpace cs = eImageColorSpace::Unknown) const = 0;
	virtual ImagePtr loadImage(const QString &path, bool fallback, eImageColorSpace cs = eImageColorSpace::Unknown) = 0;
	virtual QByteArray saveImage(ImagePtr image, const QString & formatExtension, float quality = 1.f) const = 0;

	virtual void reloadImages() = 0;

	virtual ImagePtr convertImage(ImagePtr image, eImageFormat fmt, eImageDataType type, eImageColorSpace colorSpace, eAlphaProcessing processing = eAlphaProcessing::None) const = 0;
	virtual ImagePtr scaleImage(ImagePtr image, int width, int height, eImageFormat format, eImageDataType type, eScaleFilter filter) const = 0;

	virtual QString getPreviewBase64(const QString & path, int maxSize, int minSize) = 0;

signals:
	void beforeImageUpdate(const QString & path);
	void afterImageUpdate(const QString & path);
	void imageUpdated(const IImage *);
};
