#pragma once

enum class eLoggingMode {
	None,
	File,
	FileAndConsole,
	Console,
};

class ILog
{
protected:
	virtual ~ILog() {}

public:
	virtual void information(const QString &info) const = 0;
	virtual void warning(const QString &warning) const = 0;
	virtual void error(const QString &error) const = 0;
};
