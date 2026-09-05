#pragma once

#include <QImage>
#include "../Utils/FileUtils.h"

class ITexture;
struct ModelSavingContext;
struct ModelLoadingContext;

class SHAREDLIB_EXPORT IParameter : public QObject
{
	Q_OBJECT

protected:
	virtual ~IParameter() {}

public:
	virtual bool isEnabled() const = 0;
	virtual bool isVisible() const = 0;

signals:
	void changed();
};

class IColorParameter : public IParameter
{
public:
	virtual const glm::vec3 & get() const = 0; // sRGB color space
	virtual void set(const glm::vec3 & sRGB) = 0;

	virtual void _set(const glm::vec3 & sRGB) = 0;

	virtual ITexture * texture() = 0;
	virtual const ITexture * texture() const = 0;
};

class IScalarParameter : public IParameter
{
public:
	virtual float min() const = 0;
	virtual float max() const = 0;
	virtual int precision() const = 0;

	virtual float get() const = 0;
	virtual void set(float) = 0;

	virtual void _set(float) = 0;

	virtual ITexture * texture() = 0;
	virtual const ITexture * texture() const = 0;
};

class IBooleanParameter : public IParameter
{
public:
	virtual bool get() const = 0;
	virtual void set(bool) = 0;

	virtual void _set(bool) = 0;
};

class IIntegerParameter : public IParameter
{
public:
	virtual int min() const = 0;
	virtual int max() const = 0;

	virtual int get() const = 0;
	virtual void set(int) = 0;

	virtual void _set(int) = 0;
};

class IVec2Parameter : public IParameter
{
public:
	virtual int precision() const = 0;

	virtual const glm::vec2 & get() const = 0;
	virtual void set(const glm::vec2 &) = 0;

	virtual void _set(const glm::vec2 &) = 0;
};

class IVec3Parameter : public IParameter
{
public:
	virtual int precision() const = 0;

	virtual const glm::vec3 & get() const = 0;
	virtual void set(const glm::vec3 &) = 0;

	virtual void _set(const glm::vec3 &) = 0;
};

class IEnumParameter : public IParameter
{
public:
	virtual int count() const = 0;
	virtual QString getTitle(int) const = 0;

	virtual int getIndex() const = 0;
	virtual void setIndex(int) = 0;

	virtual void _setIndex(int) = 0;
};

class IStringParameter : public IParameter
{
public:
	virtual const QString & get() const = 0;
	virtual void set(const QString &) = 0;

	virtual void _set(const QString &) = 0;
};

class ITimeParameter : public IParameter
{
public:
	virtual int get() const = 0;
	virtual void set(int) = 0;

	virtual const QString format() const = 0;

	virtual int min() const = 0;
	virtual int max() const = 0;
	virtual int step() const = 0;
};

class IFileNameParameter : public IParameter
{
public:
	virtual const FileFormatsList & extensions() const = 0;

	virtual const QString & get() const = 0;
	virtual void set(const QString &) = 0;
};

class SHAREDLIB_EXPORT ITexture : public QObject
{
	Q_OBJECT
protected:
	virtual ~ITexture() {}

public:
	enum eType {
		TYPE_COLOR,
		TYPE_SCALAR,
		TYPE_BUMP_MAP,
		// TYPE_COLOR_sRGB,
	};

	enum eWrap {
		WRAP_REPEAT,
		WRAP_CLAMP,
		WRAP_MIRROR,
	};

	enum eMapping {
		MAPPING_UV0,
		MAPPING_UV1,
		MAPPING_UV2,
		MAPPING_UV3,
	};

	virtual IBooleanParameter & enabled() = 0;

	virtual IFileNameParameter & fileName() = 0;
	virtual const IFileNameParameter & fileName() const = 0;

	virtual IBooleanParameter & normalMap() = 0;

	virtual IBooleanParameter & invert() = 0;

	virtual IEnumParameter & mapping() = 0;

	virtual IVec2Parameter & repeat() = 0;

	virtual IVec2Parameter & offset() = 0;

	virtual IScalarParameter & rotation() = 0;

	virtual IScalarParameter & cropLeft() = 0;
	virtual IScalarParameter & cropTop() = 0;
	virtual IScalarParameter & cropRight() = 0;
	virtual IScalarParameter & cropBottom() = 0;

	virtual IEnumParameter & wrapX() = 0;
	virtual IEnumParameter & wrapY() = 0;

	virtual IScalarParameter & brightness() = 0;
	virtual IScalarParameter & contrast() = 0;
	virtual IScalarParameter & gamma() = 0;

	virtual bool isEmpty() const = 0;

	virtual bool load(const QByteArray &data, const ModelLoadingContext &ctx) = 0;
	virtual QByteArray save(const ModelSavingContext &ctx) const = 0;

	virtual QImage getPreview() const = 0;

	virtual void _set(bool enabled, const QString & fileName, const RectF & crop = RectF(0.f, 0.f, 1.f, 1.f),
		eWrap wrapX = WRAP_REPEAT, eWrap wrapY = WRAP_REPEAT, eMapping mapping = MAPPING_UV0,
		const glm::vec2 & repeat = glm::vec2(1.f), const glm::vec2 & offset = glm::vec2(0.f), float rotation = 0.f,
		bool invert = false, float brightness = 0.f, float contrast = 0.f, float gamma = 1.f,
		bool normalMap = false) = 0;
};
