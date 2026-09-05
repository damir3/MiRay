#include "UVMappingProxy.h"

UVMappingProxy::UVMappingProxy(std::unique_ptr<IUVMapping> uvMapping, QQmlContext * context)
	: m_uvMapping(std::move(uvMapping))
	, m_context(context)
	, m_mapping("Mapping")
	, m_fitTo("Fit to")
	, m_uvSet("UV set")
	, m_flipU("Flip U")
	, m_flipV("Flip V")
	, m_normalize("Normalize")
	, m_scale("Scale")
	, m_repeat("Repeat")
{
	m_mapping.setParam(&m_uvMapping->mapping());
	m_fitTo.setParam(&m_uvMapping->fitTo());
	m_uvSet.setParam(&m_uvMapping->uvset());
	m_flipU.setParam(&m_uvMapping->flipU());
	m_flipV.setParam(&m_uvMapping->flipV());
	m_normalize.setParam(&m_uvMapping->normalize());
	m_scale.setParam(&m_uvMapping->scale());
	m_repeat.setParam(&m_uvMapping->repeat());

	m_data["mapping"] = QVariant::fromValue(static_cast<QObject *>(&m_mapping));
	m_data["fitTo"] = QVariant::fromValue(static_cast<QObject *>(&m_fitTo));
	m_data["uvSet"] = QVariant::fromValue(static_cast<QObject *>(&m_uvSet));
	m_data["flipU"] = QVariant::fromValue(static_cast<QObject *>(&m_flipU));
	m_data["flipV"] = QVariant::fromValue(static_cast<QObject *>(&m_flipV));
	m_data["normalize"] = QVariant::fromValue(static_cast<QObject *>(&m_normalize));
	m_data["scale"] = QVariant::fromValue(static_cast<QObject *>(&m_scale));
	m_data["repeat"] = QVariant::fromValue(static_cast<QObject *>(&m_repeat));

	m_context->setContextProperty("uvMappingModel", m_data);
}

UVMappingProxy::~UVMappingProxy()
{
	m_context->setContextProperty("uvMappingModel", nullptr);
}

void UVMappingProxy::accept()
{
	m_uvMapping->accept();
}
