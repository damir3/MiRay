#pragma once

class AppLog : public ILog
{
	enum class eLogType {
		Information,
		Warning,
		Error,
	};

	eLoggingMode	m_loggingMode;

	mutable QMutex		m_mutex;
	mutable QFile		m_file;
	mutable QString		m_logLocation;

	void log(eLogType type, const QString &message) const;

public:
	AppLog(eLoggingMode mode);
	~AppLog();

	void information(const QString &info) const override;
	void warning(const QString &warning) const override;
	void error(const QString &error) const override;
};
