#include "AppLog.h"

#include "../../Shared/Utils/FileUtils.h"

AppLog::AppLog(eLoggingMode mode)
	: m_loggingMode(mode)
{
}

AppLog::~AppLog()
{
}

void AppLog::log(eLogType type, const QString &message) const
{
	if (m_loggingMode == eLoggingMode::None)
		return;

#ifdef Q_OS_MACX
	bool copyToConsole = true;
#else
	bool copyToConsole = m_loggingMode == eLoggingMode::Console || m_loggingMode == eLoggingMode::FileAndConsole;
#endif
	bool copyToFile = m_loggingMode == eLoggingMode::FileAndConsole || m_loggingMode == eLoggingMode::File;

	if (copyToConsole)
		QTextStream(stdout) << message << '\n';

	if (!copyToFile)
		return;

	if (!m_file.isOpen()) {
		if (m_logLocation.isEmpty()) {
			QString dir = standardTempLocation() + QString::fromUtf8("/miray");
			QDir dirCache(dir);
			if (!dirCache.exists()) dirCache.mkpath(dir);
			m_logLocation = dirCache.absoluteFilePath("miray_log.txt");
		}

		m_file.setFileName(m_logLocation);
		m_file.open(QFile::WriteOnly | QFile::Truncate | QFile::Unbuffered | QFile::Text);

		assert(m_file.isOpen());
		if (!m_file.isOpen()) return;
	}

	QString typeStr;
	switch (type) {
		case eLogType::Information: typeStr = "INFO"; break;
		case eLogType::Warning: typeStr = "WARNING"; break;
		case eLogType::Error: typeStr = "ERROR"; break;
	}

	QMutexLocker locker(&m_mutex);

	QString prefix = QString("%1 [%2]: ").arg(QTime::currentTime().toString()).arg(typeStr);

	assert(m_file.isOpen());

	QStringList lines = message.split("\n");
	for (auto s : lines)
		m_file.write(QString("%1%2\n").arg(prefix).arg(s).toUtf8());

#if defined Q_OS_WIN && defined _DEBUG
	for (auto s : lines)
		OutputDebugStringW((LPCTSTR)QString("%1%2\n").arg(prefix).arg(s).utf16());
#endif

	m_file.flush();
}

void AppLog::information(const QString &info) const
{
	log(eLogType::Information, info);
}

void AppLog::warning(const QString &warning) const
{
	log(eLogType::Warning, warning);
}

void AppLog::error(const QString &error) const
{
	log(eLogType::Error, error);
}
