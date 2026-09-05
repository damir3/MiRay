#include "FitToViewProxy.h"

FitToViewProxy::FitToViewProxy(std::unique_ptr<IFitToView> fitToView, QQmlContext * context)
	: m_fitToView(std::move(fitToView))
	, m_context(context)
	, m_justSelectedObjects("Just selected objects")
	, m_includingChildrenObjects("Including children objects")
	, m_keepAspect("Keep aspect")
	, m_padding("Padding")
{
	m_justSelectedObjects.setParam(&m_fitToView->justSelectedObjects());
	m_includingChildrenObjects.setParam(&m_fitToView->includingChildrenObjects());
	m_keepAspect.setParam(&m_fitToView->keepAspect());
	m_padding.setParam(&m_fitToView->padding());

	m_data["justSelectedObjects"] = QVariant::fromValue(static_cast<QObject *>(&m_justSelectedObjects));
	m_data["includingChildrenObjects"] = QVariant::fromValue(static_cast<QObject *>(&m_includingChildrenObjects));
	m_data["keepAspect"] = QVariant::fromValue(static_cast<QObject *>(&m_keepAspect));
	m_data["padding"] = QVariant::fromValue(static_cast<QObject *>(&m_padding));

	m_context->setContextProperty("fitToViewModel", m_data);
}

FitToViewProxy::~FitToViewProxy()
{
	m_context->setContextProperty("fitToViewModel", nullptr);
}

void FitToViewProxy::accept()
{
	m_fitToView->accept();
}
