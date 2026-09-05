#pragma once

class SHAREDLIB_EXPORT BaseLogger
{
	QtMsgType m_type;
	mutable QByteArray m_data;

	QMessageLogContext m_context;

protected:
	BaseLogger(QtMsgType type);
	~BaseLogger();

public:
	const BaseLogger& operator<<(const QString&) const;
};

class SHAREDLIB_EXPORT LogInformation : public BaseLogger
{
public:
	LogInformation();
};

class SHAREDLIB_EXPORT LogWarning : public BaseLogger
{
public:
	LogWarning();
};

class SHAREDLIB_EXPORT LogError : public BaseLogger
{
public:
	LogError();
};
