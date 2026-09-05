#pragma once

#include "ParamProxy.h"

#include "../Shared/Interfaces/Camera.h"

class FitToViewProxy
{
	std::unique_ptr<IFitToView> m_fitToView;
	QQmlContext * const	m_context;

	BooleanParamProxy	m_justSelectedObjects;
	BooleanParamProxy	m_includingChildrenObjects;
	BooleanParamProxy	m_keepAspect;
	ScalarParamProxy	m_padding;

	QVariantMap	m_data;

public:
	FitToViewProxy(std::unique_ptr<IFitToView> fitToView, QQmlContext * context);
	~FitToViewProxy();

	void accept();
};
