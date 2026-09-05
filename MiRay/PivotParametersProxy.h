#pragma once

#include "ParamProxy.h"

#include "../Shared/Interfaces/Scene.h"

class PivotParametersProxy
{
	std::unique_ptr<IPivotParameters> m_pivotParameters;
	QQmlContext * const	m_context;

	EnumParamProxy		m_calculatePivot;
	BooleanParamProxy	m_includingChildrenNodes;
	EnumParamProxy		m_pivotX;
	EnumParamProxy		m_pivotY;
	EnumParamProxy		m_pivotZ;

	QVariantMap	m_data;

public:
	PivotParametersProxy(std::unique_ptr<IPivotParameters> pivotParameters, QQmlContext * context);
	~PivotParametersProxy();

	void accept();
};
