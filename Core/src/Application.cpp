#include "../Application.h"

#include "MainFactory.h"

#include "ImageManager.h"
#include "ModelLoadersRegistry.h"
#include "ModelSaversRegistry.h"
#include "AppLog.h"
#include "Settings.h"

#include "MiRaySceneLoader.h"
#include "MiRaySceneSaver.h"
#include "../../Shared/Utils/MetaTypes.h"
#include "../../Shared/Utils/FileUtils.h"

#include "Materials/IndexOfRefractionImpl.h"

void customMessageOutput(QtMsgType type, const QMessageLogContext&, const QString& msg)
{
	const auto log = qApp->log();
	if (!log)
		return;

	switch (type) {
		case QtDebugMsg:
			log->information(msg);
			break;
		case QtWarningMsg:
			log->warning(msg);
			break;
		case QtCriticalMsg:
			log->error(msg);
			break;
		case QtFatalMsg:
			log->error(msg);
			abort();
	}
}

Application::Application(int &argc, char **argv)
	: QApplication(argc, argv)
	, m_resourcesFolderPath(getResourcesFolder().absolutePath())
{
	qInstallMessageHandler(customMessageOutput);

	qRegisterMetaType<::eSceneInternalModification>("eSceneInternalModification");
	customRegisterMetaType<MiRaySceneLoader>();
	customRegisterMetaType<MiRaySceneSaver>();

	IndexOfRefractionImpl::initSpectrumColors();

	m_mapOldNewResources["Library/Textures/Environment/Environment 1.hdr"] = "Library/Environments/Basic/Environment 01.exr";
	m_mapOldNewResources["Library/Textures/Environment/Environment 2.hdr"] = "Library/Environments/Basic/Environment 02.exr";
	m_mapOldNewResources["Library/Textures/Environment/Studio 1.hdr"] = "Library/Environments/Basic/Studio 1.exr";
	m_mapOldNewResources["Library/Textures/Environment/Studio 2.hdr"] = "Library/Environments/Basic/Studio 2.exr";
	m_mapOldNewResources["Library/Textures/Environment/Studio 3.hdr"] = "Library/Environments/Basic/Studio 3.exr";
	m_mapOldNewResources["Library/Textures/Environment/Studio 4.hdr"] = "Library/Environments/Basic/Studio 4.exr";
	m_mapOldNewResources["Library/Textures/Environment/Studio 5.hdr"] = "Library/Environments/Basic/Studio 5.exr";
}

void Application::init(eLoggingMode mode)
{
	m_mainFactory.reset(new MainFactory());
	m_log.reset(new AppLog(mode));
	m_settings.reset(new Settings());

	m_images.reset(new ImageManager(*m_mainFactory.get(), *m_log.get()));

	m_modelLoadersRegistry.reset(new ModelLoadersRegistry(*m_mainFactory.get()));
	m_modelSaversRegistry.reset(new ModelSaversRegistry(*m_mainFactory.get()));

	setVersion();
}

Application::~Application()
{
	if (auto l = log())
		l->information("Application closed");
}

void Application::startPlugins()
{
	m_plugins.loadPlugins(*m_log.get());
	m_plugins.activateInContext(this);
	m_modelLoadersRegistry->registerModelLoader(ModelLoadingFormat{ MiRaySceneLoader::uid(), "MiRay Scene Files", { "mirayScene", "owletScene" } });
	m_modelSaversRegistry->registerModelSaver(ModelSavingFormat{ MiRaySceneSaver::uid(), "MiRay Scene Files", { "mirayScene" } }, false);
}

IImageManager *Application::imageManager() { return m_images.get(); }
IModelLoaderRegistry *Application::modelLoadersRegistry() { return m_modelLoadersRegistry.get(); }
IModelSaverRegistry *Application::modelSaversRegistry() { return m_modelSaversRegistry.get(); }
ILog *Application::log() { return m_log.get(); }
ISettings *Application::settings() { return m_settings.get(); }

ICoreInstance	*Application::createCoreInstance()
{
	return new CoreInstance(*this);
}

IApplicationContext::MaterialInformation Application::loadMaterialInformation(const QByteArray &mtl_data) const
{
	MaterialInformation info;

	// material?
	if (MaterialImpl::getMaterialInformation(mtl_data, info))
		return info;

	// image?
	auto image = m_images->loadImage(mtl_data, QString(), false, eImageColorSpace::sRGB);
	if (!image)
		return MaterialInformation();

	const int MAX = 512;
	if (image->width() > MAX || image->height() > MAX) {
		int w, h;
		if (image->width() >= image->height()) {
			w = MAX;
			h = MAX * image->height() / image->width();
		} else {
			h = MAX;
			w = MAX * image->width() / image->height();
		}
		image = m_images->scaleImage(image, w, h, eImageFormat::RGBA, eImageDataType::Byte, eScaleFilter::Triangle);
	} else {
		image = m_images->convertImage(image, eImageFormat::RGBA, eImageDataType::Byte, eImageColorSpace::Unknown);
	}

	info.thumbnail = image->toQImage(false);

	return info;
}

// ------------------------------------------------------------------------ //

void Application::setVersion()
{
	QString version;
	if (VER_BUILD > 0)
		version = QString("%1.%2.%3").arg(VER_MAJOR).arg(VER_MINOR).arg(VER_BUILD);
	else
		version = QString("%1.%2").arg(VER_MAJOR).arg(VER_MINOR);
	setApplicationVersion(version);
	setOrganizationDomain("github.com/damir3/MiRay2");
}

QDir Application::getResourcesFolder() const
{
	QDir dir(applicationDirPath());
#ifdef Q_OS_MACX
	dir.cd("../Resources");
#endif
	return dir;
}

QString Application::getLocalResourcePath(const QString &fileName) const
{
	if (fileName.indexOf("miray:///") == 0 || fileName.indexOf("miray:\\\\\\") == 0)
		return fileName.mid(9);

	if (fileName.indexOf("miray://") == 0 || fileName.indexOf("miray:\\\\") == 0)
		return fileName.mid(8);

	if (fileName.indexOf("owlet:///") == 0 || fileName.indexOf("owlet:\\\\\\") == 0)
		return fileName.mid(9);

	if (fileName.indexOf("owlet://") == 0 || fileName.indexOf("owlet:\\\\") == 0)
		return fileName.mid(8);

	return QString();
}

QString Application::getShortResourcePath(const QString& fileName) const
{
	auto local = getLocalResourcePath(fileName);
	if (!local.isEmpty())
		return QString("miray://%1").arg(local.replace('\\', '/'));

	const auto filePath = urlToLocalFile(fileName);
	const auto absResPath = getResourcesFolder().absolutePath();
	const auto absFilePath = QFileInfo(filePath).absoluteFilePath();
	if (absFilePath.startsWith(absResPath)) {
		const auto relPath = QDir(absResPath).relativeFilePath(absFilePath).replace('\\', '/');
		return QString("miray://%1").arg(relPath);
	}

	return filePath;
}

QString Application::getFullResourcePath(const QString &fileName) const
{
//	qDebug() << "GetFullResourcePath" << fileName;
	auto local = getLocalResourcePath(fileName);
	if (local.isEmpty())
		return fileName;

	// here we do mapping of old filenames to the new ones
	local.replace('\\', '/');
	if (m_mapOldNewResources.contains(local))
		local = m_mapOldNewResources[local];

//	qDebug() << "GetFullResourcePath" << QDir::toNativeSeparators(getResourcesFolder().absoluteFilePath(local));
	return QDir::toNativeSeparators(getResourcesFolder().absoluteFilePath(local));
}

bool Application::isInternalResource(const QString &fileName) const
{
	const auto path = urlToLocalFile(fileName);
	return
		path.startsWith(m_resourcesFolderPath) ||
		path.indexOf("miray://") == 0 || path.indexOf("miray:\\\\") == 0 ||
		path.indexOf("owlet://") == 0 || path.indexOf("owlet:\\\\") == 0;
}

QString Application::checkHardware()
{
	auto dev = rtcNewDevice(nullptr);
	auto err = rtcGetDeviceError(dev);
	rtcReleaseDevice(dev);

	if (err != RTC_ERROR_NONE) {
		if (err == RTC_ERROR_UNSUPPORTED_CPU)
			return "Your CPU is not supported";
		return QString("Embree initialization failed with code %1").arg(err);
	}

	return QString();
}
