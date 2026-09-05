#pragma once

#include "../../Shared/Interfaces/Settings.h"

class Settings : public ISettings
{
	mutable QSettings m_storage;

public:
	Settings();

	void setValue(const QString &name, const QVariant &val);
	QVariant getValue(const QString &name, const QVariant &def) const;

	QStringList groups(const QString&) const;
};
