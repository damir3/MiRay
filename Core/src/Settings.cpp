#include "Settings.h"

Settings::Settings()
	: m_storage("TuiSoftware", "MiRay")
{
}

void Settings::setValue(const QString &name, const QVariant &val)
{
	if (val.type() == QVariant::Bool) { // see #809
		m_storage.setValue(name, val.toBool() ? 1 : 0);
	} else {
		m_storage.setValue(name, val);
	}
	emit valueChanged(name, val);
}

QVariant Settings::getValue(const QString &name, const QVariant &def) const
{
	return m_storage.value(name, def);
}

QStringList Settings::groups(const QString& path) const
{
	auto _path = path.split("/");
	for (const auto& group : _path) {
		m_storage.beginGroup(group);
	}

	auto groupList = m_storage.childGroups();

	for (int i = 0; i < _path.length(); i++)
		m_storage.endGroup();

	return groupList;
}
