#pragma once

class SHAREDLIB_EXPORT ISettings : public QObject
{
	Q_OBJECT

protected:
	virtual ~ISettings() {}

public:
	virtual void setValue(const QString &name, const QVariant &val) = 0;
	virtual QVariant getValue(const QString &name, const QVariant &def) const = 0;

	virtual QStringList groups(const QString&) const = 0;

signals:
	void valueChanged(const QString &name, const QVariant &val);
};

#define JOB_MANAGER_QUEUED_JOBS_PATH "jobManagerQueuedJobsPath"
#define SETTINGS_OPEN_GL_DRIVER "OpenGLDriver"
#define SETTINGS_PREVIEW_DENOISE "PreviewDenoise"
#define SETTINGS_PREVIEW_FRAMES "PreviewFrames"

#define SETTINGS_UPDATE_CHECK_DISABLED			"UpdateChecker/Disabled"
#define SETTINGS_UPDATE_NOTIFY					"UpdateChecker/CheckForUpdates"
#define SETTINGS_UPDATE_NOTIFY_INCLUDING_BETAS	"UpdateChecker/IncludingBetas"
#define SETTINGS_UPDATE_LAST					"UpdateChecker/LastUpdateChecked"

#define SETTINGS_PUT_LOADED_MODELS_ON_THE_FLOOR	"PutLoadedModelsOnTheFloor"

enum {
	PREVIEW_FRAMES_DEFAULT = 24,
	PREVIEW_FRAMES_MIN = 8,
	PREVIEW_FRAMES_MAX = 128,
	PREVIEW_DENOISE_DEFAULT = true,
	DEFAULT_CHECK_UPDATES = true,
	DEFAULT_CHECK_UPDATES_INCLUDING_BETAS = false,
	DEFAULT_PUT_LOADED_MODELS_ON_THE_FLOOR = true,
};
