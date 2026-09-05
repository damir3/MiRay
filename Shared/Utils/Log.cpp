#include "Log.h"

// ------------------------------------------------------------------------ //
// Base logging stuff

BaseLogger::BaseLogger(QtMsgType type)
	: m_type(type)
{
}

BaseLogger::~BaseLogger()
{
	qt_message_output(m_type, m_context, m_data.data());
}

const BaseLogger& BaseLogger::operator<<(const QString& strMsg) const
{
	if (!m_data.isEmpty())
		m_data.append(" ");

	m_data.append(strMsg.toUtf8());

	return *this;
}

// ------------------------------------------------------------------------ //
// Implemenations

LogInformation::LogInformation() : BaseLogger(QtDebugMsg) {}
LogWarning::LogWarning() : BaseLogger(QtWarningMsg) {}
LogError::LogError() : BaseLogger(QtCriticalMsg) {}
