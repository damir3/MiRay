#pragma once

#include <QScopedPointer>

class QObject;
class QString;

class IObjectsFactory
{
protected:
	virtual ~IObjectsFactory() {}

public:
	virtual QScopedPointer<QObject> createByName(const QString & name) = 0;
};
