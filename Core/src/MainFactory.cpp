#include "MainFactory.h"

QScopedPointer<QObject> MainFactory::createByName(const QString &name)
{
	int idx = QMetaType::type(name.toLower().toUtf8().data());
	if (idx > 0) {
		void *p = QMetaType::create(idx);
		if (p)
			return QScopedPointer<QObject>(reinterpret_cast<QObject *>(p));
	}

	throw std::runtime_error(QString("Can't create object of type %1").arg(name).toStdString());
}
