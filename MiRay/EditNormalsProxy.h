#pragma once

#include "ParamProxy.h"

#include "../Shared/Interfaces/Scene.h"

class EditNormalsProxy
{
	std::unique_ptr<IEditNormals> m_editNormals;
	QQmlContext * const	m_context;

	BooleanParamProxy	m_calculateNormals;
	EnumParamProxy		m_makeEdges;
	ScalarParamProxy	m_maxSoftAngle;
	BooleanParamProxy	m_flipNormals;
	BooleanParamProxy	m_flipFacing;

	QVariantMap	m_data;

public:
	EditNormalsProxy(std::unique_ptr<IEditNormals> editNormals, QQmlContext * context);
	~EditNormalsProxy();

	void accept();
};
