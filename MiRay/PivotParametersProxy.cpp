#include "PivotParametersProxy.h"

PivotParametersProxy::PivotParametersProxy(std::unique_ptr<IPivotParameters> pivotParameters, QQmlContext * context)
	: m_pivotParameters(std::move(pivotParameters))
	, m_context(context)
	, m_calculatePivot("Calculate pivot")
	, m_includingChildrenNodes("Including children nodes")
	, m_pivotX("Pivot X axis")
	, m_pivotY("Pivot Y axis")
	, m_pivotZ("Pivot Z axis")
{
	m_calculatePivot.setParam(&m_pivotParameters->calculatePivot());
	m_includingChildrenNodes.setParam(&m_pivotParameters->includingChildrenNodes());
	m_pivotX.setParam(&m_pivotParameters->pivotX());
	m_pivotY.setParam(&m_pivotParameters->pivotY());
	m_pivotZ.setParam(&m_pivotParameters->pivotZ());

	m_data["calculatePivot"] = QVariant::fromValue(static_cast<QObject *>(&m_calculatePivot));
	m_data["includingChildrenNodes"] = QVariant::fromValue(static_cast<QObject *>(&m_includingChildrenNodes));
	m_data["pivotX"] = QVariant::fromValue(static_cast<QObject *>(&m_pivotX));
	m_data["pivotY"] = QVariant::fromValue(static_cast<QObject *>(&m_pivotY));
	m_data["pivotZ"] = QVariant::fromValue(static_cast<QObject *>(&m_pivotZ));

	m_context->setContextProperty("pivotParamsModel", m_data);
}

PivotParametersProxy::~PivotParametersProxy()
{
	m_context->setContextProperty("pivotParamsModel", nullptr);
}

void PivotParametersProxy::accept()
{
	m_pivotParameters->accept();
}
