#pragma once

#include <QVariant>
#include "../Utils/FileUtils.h"

class IScene;
class INode;
class IMaterialManager;
class IMaterial;
class QDomElement;
struct ModelLoadingContext;

struct ModelLoadingFormat {
	QString	id;
	QString	description;
	QStringList	extensions; // just extensions, no dots
};

class IMaterialCreator
{
public:
	virtual ~IMaterialCreator() {}

	virtual IMaterial *create(const QString &name, const QJsonObject &params, const ModelLoadingContext &ctx) = 0;
	virtual IMaterial *create(const QString &name, const QDomElement &data, const ModelLoadingContext &ctx) = 0;
};

#define MATERIAL_PARAMETER_OPACITY_FACTOR			"opacity_factor"
#define MATERIAL_PARAMETER_DIFFUSE_TEXTURE			"diffuse_texture"
#define MATERIAL_PARAMETER_GLOSSINESS_TEXTURE		"glossiness_texture"
#define MATERIAL_PARAMETER_SPECULAR_TEXTURE			"specular_texture"
#define MATERIAL_PARAMETER_DIFFUSE_COLOR			"diffuse_color"
#define MATERIAL_PARAMETER_SPECULAR_COLOR			"specular_color"
#define MATERIAL_PARAMETER_AMBIENT_COLOR			"ambient_color"
#define MATERIAL_PARAMETER_EMISSIVE_COLOR			"emissive_color"
#define MATERIAL_PARAMETER_BUMP_TEXTURE				"bump_texture"
#define MATERIAL_PARAMETER_BUMP_NORMAL_MAP			"bump_normal_map"
#define MATERIAL_PARAMETER_BUMP_FACTOR				"bump_factor"
#define MATERIAL_PARAMETER_REFRACTION_N				"refraction_n"
#define MATERIAL_PARAMETER_REFRACTION_K				"refraction_k"
#define MATERIAL_PARAMETER_REFLECTION_BLUR			"reflection_blur"
#define MATERIAL_PARAMETER_REFLECTION_BLUR_MASK		"reflection_blur_texture"

class SHAREDLIB_EXPORT IModelLoader : public QObject
{
	Q_OBJECT

public:
	virtual ~IModelLoader() {}

	virtual bool canLoad(const ModelLoadingContext &ctx) const = 0;
	virtual bool needsAutoSetup() const = 0;
};

class ISteppedModelLoader
{
protected:
	virtual ~ISteppedModelLoader() {}

public:
	virtual void preLoad(ModelLoadingContext &ctx, IScene *scene) = 0;
	virtual int stepCount() const = 0;
	virtual void loadStep(const ModelLoadingContext &ctx, int i, IMaterialCreator *materialCreator) = 0;
};

class INodeImporter
{
protected:
	virtual ~INodeImporter() {}

public:
	virtual void importNodes(INode *dest, IScene *scene, const QByteArray &data) = 0;
};

class IProgressCallback
{
protected:
	virtual ~IProgressCallback() {}

public:
	virtual bool onProgress(double progress) = 0;
};

class ICallbackModelLoader
{
protected:
	virtual ~ICallbackModelLoader() {}

public:
	virtual void load(const ModelLoadingContext &ctx, IScene *scene, IMaterialCreator *materialCreator, IProgressCallback *callback) = 0;
};

Q_DECLARE_INTERFACE(ISteppedModelLoader, "org.miray.ISteppedModelLoader")
Q_DECLARE_INTERFACE(ICallbackModelLoader, "org.miray.ICallbackModelLoader")
Q_DECLARE_INTERFACE(INodeImporter, "org.miray.INodeImporter")

typedef QList<ModelLoadingFormat>	ListOfModelLoadingFormats;

class SHAREDLIB_EXPORT IModelLoaderRegistry
{
protected:
	virtual ~IModelLoaderRegistry() {}

public:
	virtual void registerModelLoader(const ModelLoadingFormat &fmt) = 0;

	virtual const ListOfModelLoadingFormats &allFormats() const = 0;

	virtual const FileFormatsList & loadingFilters() const = 0;

	// runs through all the registered types, checks by extensions and then call IModelLoader::canLoad() to double-check that the format is supported
	virtual bool canLoadFile(const QString &fileName, bool deeperCheck) const = 0;

	// tries all the loaders' canLoad() and returns the first that accepted the data
	virtual std::unique_ptr<IModelLoader> getLoader(const QString &fileName) const = 0;
};
