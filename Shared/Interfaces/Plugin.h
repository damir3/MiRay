#pragma once

class IApplicationContext;

class SHAREDLIB_EXPORT IPlugin : public QObject
{
	Q_OBJECT

	IPlugin &operator = (const IPlugin &ref);

protected:
	IPlugin() {}

public:
	virtual void activateInContext(IApplicationContext *appContext) = 0;
};

Q_DECLARE_INTERFACE(IPlugin, "org.miray.IPlugin")
