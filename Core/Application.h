#pragma once

#include "../Shared/Interfaces/ApplicationContext.h"
#include "../Shared/Interfaces/Log.h"
#include "PluginManager.h"

class ImageManager;
class ModelLoadersRegistry;
class ModelSaversRegistry;
class MainFactory;
class AppLog;
class Settings;

class IObjectsFactory;
class ICoreInstance;

class CORE_EXPORT Application
	: public QApplication
	, public IApplicationContext
{
	Q_OBJECT
	Q_INTERFACES(IApplicationContext)

	PluginManager m_plugins;
	QMap<QString, QString> m_mapOldNewResources;
	const QString m_resourcesFolderPath;

	std::unique_ptr<ImageManager>			m_images;
	std::unique_ptr<MainFactory>			m_mainFactory;
	std::unique_ptr<ModelLoadersRegistry>	m_modelLoadersRegistry;
	std::unique_ptr<ModelSaversRegistry>	m_modelSaversRegistry;
	std::unique_ptr<AppLog>					m_log;
	std::unique_ptr<Settings>				m_settings;

	void setVersion();

	QString getLocalResourcePath(const QString &fileName) const;

public:
	Application(int &argc, char **argv);
	~Application() override;

	void startPlugins();

	IImageManager *imageManager() override;
	IModelLoaderRegistry *modelLoadersRegistry() override;
	IModelSaverRegistry *modelSaversRegistry() override;
	ILog *log() override;
	ISettings *settings() override;

	QDir getResourcesFolder() const override;
	QString getShortResourcePath(const QString& fileName) const override;
	QString getFullResourcePath(const QString &fileName) const override;
	bool isInternalResource(const QString &fileName) const override;

	ICoreInstance *createCoreInstance() override;

	MaterialInformation loadMaterialInformation(const QByteArray &mtl_data) const override;

	virtual void init(eLoggingMode mode);

	static QString checkHardware();
};

#if defined(qApp)
	#undef qApp
#endif
#define qApp (static_cast<Application *>(QCoreApplication::instance()))
