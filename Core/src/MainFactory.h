#pragma once

#include "../../Shared/Interfaces/ObjectsFactory.h"

class MainFactory : public IObjectsFactory
{
public:
	QScopedPointer<QObject> createByName(const QString & name) override;
};
