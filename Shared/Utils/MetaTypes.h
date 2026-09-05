#pragma once

template <typename T>
void *customMetaTypeConstructHelper(void *where, const void *copy)
{
	if (copy) {
		assert(false); // copy-constructors are not supported for custom metatypes
		return NULL;
	}
	return new (where)T;
}

template <typename T>
int customRegisterMetaType()
{
	auto normalizedTypeName = QByteArray(T::uid()).toLower();

	const int typedefOf = QtPrivate::QMetaTypeIdHelper<T>::qt_metatype_id();
	if (typedefOf != -1) {
		assert(false); // remove Q_DECLARE_METATYPE(T)
		return -1;
	}

	QMetaType::TypeFlags flags(QtPrivate::QMetaTypeTypeFlags<T>::Flags);

	const int id = QMetaType::registerNormalizedType(normalizedTypeName,
		QtMetaTypePrivate::QMetaTypeFunctionHelper<T>::Destruct,
		customMetaTypeConstructHelper<T>,
		int(sizeof(T)),
		flags,
		QtPrivate::MetaObjectForType<T>::value());

	if (id > 0) {
		QtPrivate::SequentialContainerConverterHelper<T>::registerConverter(id);
		QtPrivate::AssociativeContainerConverterHelper<T>::registerConverter(id);
		QtPrivate::MetaTypePairHelper<T>::registerConverter(id);
		QtPrivate::MetaTypeSmartPointerHelper<T>::registerConverter(id);
	}

	return id;
}
