#include "AppSettingsProxy.h"

#ifdef Q_OS_WIN
static const QStringList g_driversNames =
{
	"Auto",
	"System OpenGL driver",
	"Direct 3D 11 emulation",
	"Direct 3D 9 emulation",
	"Direct 3D software emulation",
	"Software OpenGL driver"
};

static const QStringList g_driversIdentifiers =
{
	"",
	"system",
	"angle-d3d11",
	"angle-d3d9",
	"angle-software",
	"software"
};
#endif

AppSettingsProxy::AppSettingsProxy(ISettings& s, QQmlContext * context)
	: m_settings(s)
	, m_context(context)
	, m_putLoadedOnTheFloor(s.getValue(SETTINGS_PUT_LOADED_MODELS_ON_THE_FLOOR, DEFAULT_PUT_LOADED_MODELS_ON_THE_FLOOR).toBool())
	, m_previewDenoise(s.getValue(SETTINGS_PREVIEW_DENOISE, true).toBool())
	, m_previewFrames(std::clamp<int>(s.getValue(SETTINGS_PREVIEW_FRAMES, PREVIEW_FRAMES_DEFAULT).toInt(), PREVIEW_FRAMES_MIN, PREVIEW_FRAMES_MAX), PREVIEW_FRAMES_MIN, PREVIEW_FRAMES_MAX)
#if VER_BETA != 0
	, m_includingBetas(true)
#else
	, m_includingBetas(s.getValue(SETTINGS_UPDATE_NOTIFY_INCLUDING_BETAS, DEFAULT_CHECK_UPDATES_INCLUDING_BETAS).toBool())
#endif
	, m_checkUpdates(s.getValue(SETTINGS_UPDATE_NOTIFY, DEFAULT_CHECK_UPDATES).toBool())
	, m_queuedJobsPath(s.getValue(JOB_MANAGER_QUEUED_JOBS_PATH, "").toString())
#ifdef Q_OS_WIN
	, m_opengl(0, g_driversNames)
#endif
	, m_putLoadedOnTheFloorProxy("Put loaded models on the floor")
	, m_previewDenoiseProxy("Reduce noise")
	, m_previewFramesProxy("Frames to render")
	, m_includingBetasProxy("Including beta versions")
	, m_checkUpdatesProxy("Notify about updates")
	, m_queuedJobsPathProxy("Queued Jobs Path")
#ifdef Q_OS_WIN
	, m_openglProxy("OpenGL Driver (requires restart)")
#endif
{
#if VER_BETA != 0
	m_includingBetas.setEnabled(false);
#endif

#ifdef Q_OS_WIN
	const auto driverValue = s.getValue(SETTINGS_OPEN_GL_DRIVER, "").toString();
	auto driverIndex = g_driversIdentifiers.indexOf(driverValue);
	driverIndex = driverIndex < 0 ? 0 : driverIndex;
	m_opengl._setIndex(driverIndex);
#endif

	m_putLoadedOnTheFloorProxy.setParam(&m_putLoadedOnTheFloor);
	m_previewDenoiseProxy.setParam(&m_previewDenoise);
	m_previewFramesProxy.setParam(&m_previewFrames);
	m_includingBetasProxy.setParam(&m_includingBetas);
	m_checkUpdatesProxy.setParam(&m_checkUpdates);
	m_queuedJobsPathProxy.setParam(&m_queuedJobsPath);
#ifdef Q_OS_WIN
	m_openglProxy.setParam(&m_opengl);
#endif

	m_data["putLoadedOnTheFloor"] = QVariant::fromValue(static_cast<QObject *>(&m_putLoadedOnTheFloorProxy));
	m_data["previewDenoise"] = QVariant::fromValue(static_cast<QObject *>(&m_previewDenoiseProxy));
	m_data["previewFrames"] = QVariant::fromValue(static_cast<QObject *>(&m_previewFramesProxy));
	m_data["includingBetas"] = QVariant::fromValue(static_cast<QObject *>(&m_includingBetasProxy));
	m_data["checkUpdates"] = QVariant::fromValue(static_cast<QObject *>(&m_checkUpdatesProxy));
	m_data["queuedJobsPath"] = QVariant::fromValue(static_cast<QObject *>(&m_queuedJobsPathProxy));
#ifdef Q_OS_WIN
	m_data["opengl"] = QVariant::fromValue(static_cast<QObject *>(&m_openglProxy));
#endif

	m_context->setContextProperty("appSettingsModel", m_data);
}

AppSettingsProxy::~AppSettingsProxy()
{
	m_context->setContextProperty("appSettingsModel", nullptr);
}

void AppSettingsProxy::accept()
{
	m_settings.setValue(SETTINGS_PUT_LOADED_MODELS_ON_THE_FLOOR, m_putLoadedOnTheFloor.get());
	m_settings.setValue(SETTINGS_PREVIEW_DENOISE, m_previewDenoise.get());
	m_settings.setValue(SETTINGS_PREVIEW_FRAMES, m_previewFrames.get());
#if VER_BETA == 0
	m_settings.setValue(SETTINGS_UPDATE_NOTIFY_INCLUDING_BETAS, m_includingBetas.get());
#endif
	m_settings.setValue(SETTINGS_UPDATE_NOTIFY, m_checkUpdates.get());
	m_settings.setValue(JOB_MANAGER_QUEUED_JOBS_PATH, m_queuedJobsPath.get());
#ifdef Q_OS_WIN
	m_settings.setValue(SETTINGS_OPEN_GL_DRIVER, g_driversIdentifiers[m_opengl.getIndex()]);
#endif
}
