#pragma once

#include "Parameters.h"

struct ModelSavingContext;
struct ModelLoadingContext;

enum class IORType {
	SCALAR = 0,
	COMPLEX,
	MEASURED
};

class IIndexOfRefraction
{
protected:
	virtual ~IIndexOfRefraction() {}

public:
	virtual IEnumParameter & type() = 0;
	virtual IScalarParameter & n() = 0;
	virtual IScalarParameter & k() = 0;
	virtual IFileNameParameter & fileName() = 0;
};

class SHAREDLIB_EXPORT IMaterialLayer : public QObject
{
	Q_OBJECT

protected:
	virtual ~IMaterialLayer() {}

public:
	virtual IStringParameter & name() = 0;

	virtual IBooleanParameter & enabled() = 0;
	virtual IScalarParameter & mask() = 0;
	virtual IScalarParameter & bump() = 0;

	virtual IBooleanParameter & diffuseLayer() = 0;
	virtual IColorParameter & diffuseColor() = 0;
	virtual IScalarParameter & diffuseOpacity() = 0;
	virtual IColorParameter & diffuseTransmission() = 0;
	virtual IBooleanParameter & diffuseTextureLayerMask() = 0;

	virtual IBooleanParameter & emissiveLayer() = 0;
	virtual IColorParameter & emissiveColor() = 0;
	virtual IScalarParameter & emissiveIntensity() = 0;

	virtual IBooleanParameter & specularLayer() = 0;
	virtual IIndexOfRefraction & indexOfRefraction() = 0;
	virtual IColorParameter & reflection() = 0;
	virtual IColorParameter & transmission() = 0;
	virtual IScalarParameter & reflection90Level() = 0;
	virtual IColorParameter & reflection90() = 0;
	virtual IScalarParameter & roughness() = 0;
	virtual IScalarParameter & anisotropy() = 0;
	virtual IScalarParameter & anisotropyAngle() = 0;

	virtual IBooleanParameter & thinFilmInterference() = 0;
	virtual IScalarParameter & thickness() = 0;
	virtual IScalarParameter & minThickness() = 0;
	virtual IIndexOfRefraction & filmIndexOfRefraction() = 0;

	virtual QByteArray save(const ModelSavingContext &ctx) const = 0;

signals:
	void changed();
};

class SHAREDLIB_EXPORT IMaterialGroup : public QObject
{
	Q_OBJECT

protected:
	virtual ~IMaterialGroup() {}

public:
	virtual IStringParameter & name() = 0;

	virtual IBooleanParameter & enabled() = 0;

	virtual IScalarParameter & mask() = 0;

	virtual size_t numLayers() const = 0;
	virtual IMaterialLayer * layer(size_t i) const = 0;

	virtual IMaterialLayer * addLayer(size_t i, const QByteArray & data, const ModelLoadingContext &ctx) = 0;
	virtual void removeLayer(size_t i) = 0;
	virtual void moveLayer(size_t from, size_t to) = 0;

	virtual IMaterialLayer * _createLayer(size_t pos) = 0;

	virtual bool load(const QByteArray &, const ModelLoadingContext &ctx) = 0;
	virtual QByteArray save(const ModelSavingContext &ctx) const = 0;

signals:
	void changed();
};

enum eMediumType {
	MEDIUM_TYPE_NONE = 0,
	MEDIUM_TYPE_MANUAL,
	MEDIUM_TYPE_MEASURED,
	MEDIUM_TYPE_OPAQUE,
};

class SHAREDLIB_EXPORT IMaterial : public QObject
{
	Q_OBJECT

protected:
	virtual ~IMaterial() {}

public:
	virtual QByteArray save(const ModelSavingContext &ctx) const = 0;

	virtual void update() = 0;

	virtual QString guid() const = 0;

	virtual IStringParameter & name() = 0;
	virtual const IStringParameter & name() const = 0;

	virtual IScalarParameter & bump() = 0;

	virtual IEnumParameter & medium() = 0; // eMediumType

	virtual IIndexOfRefraction & indexOfRefraction() = 0;

	virtual IColorParameter & absorptionColor() = 0;
	virtual IScalarParameter & absorptionAttenuation() = 0;

	virtual IColorParameter & emissionColor() = 0;
	virtual IScalarParameter & emissionScale() = 0;

	virtual IBooleanParameter & subsurfaceScattering() = 0;
	virtual IColorParameter & scatteringColor() = 0;
	virtual IScalarParameter & scatteringScale() = 0;
	virtual IScalarParameter & scatteringAsymmetry() = 0;

	virtual IIntegerParameter & priority() = 0;

	virtual IBooleanParameter & doubleSided() = 0;

	// TODO: add subsurface scattering parameters

	virtual size_t numGroups() const = 0;
	virtual IMaterialGroup *group(size_t i) const = 0;

	virtual IMaterialGroup *addGroup(size_t i) = 0;
	virtual void removeGroup(size_t i) = 0;
	virtual void moveGroup(size_t from, size_t to) = 0;

	virtual IMaterialGroup * _createGroup(size_t pos) = 0;

	virtual void moveLayer(size_t groupFrom, size_t layerFrom, size_t groupTo, size_t layerTo) = 0;

	virtual bool isPreviewValid() const = 0;
	virtual QImage getPreview() const = 0;
	virtual void setPreview(QImage image) = 0;

signals:
	void changed();
};

static const char * const IOR_EXTENSIONS[] = {
	"refractiveindex.info files", "txt", "csv", "yml", nullptr,
	"filmetrics.com files", "txt", nullptr,
	"Maxwell IOR files", "ior", nullptr,
	"SOPRA IOR files", "nk", nullptr,
	nullptr,
};
