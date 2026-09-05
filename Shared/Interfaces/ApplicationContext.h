#pragma once

class IApplicationContext
{
protected:
	virtual ~IApplicationContext() {}

public:
	virtual class IImageManager *imageManager() = 0;
	virtual class IModelLoaderRegistry *modelLoadersRegistry() = 0;
	virtual class IModelSaverRegistry *modelSaversRegistry() = 0;
	virtual class ILog *log() = 0;
	virtual class ISettings *settings() = 0;

	virtual QDir getResourcesFolder() const = 0;
	virtual QString getShortResourcePath(const QString& fileName) const = 0;
	virtual QString getFullResourcePath(const QString &fileName) const = 0;
	virtual bool isInternalResource(const QString &fileName) const = 0;

	virtual class ICoreInstance *createCoreInstance() = 0;

	struct MaterialInformation {
		QString name;
		QString guid;
		QImage thumbnail;
	};

	virtual MaterialInformation loadMaterialInformation(const QByteArray &mtl_data) const = 0;
};

Q_DECLARE_INTERFACE(IApplicationContext, "org.miray.IApplicationContext")
