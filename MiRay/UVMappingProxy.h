#pragma once

#include "ParamProxy.h"

#include "../Shared/Interfaces/Scene.h"

class UVMappingProxy
{
	std::unique_ptr<IUVMapping> m_uvMapping;
	QQmlContext * const	m_context;

	EnumParamProxy		m_mapping;
	EnumParamProxy		m_fitTo;
	EnumParamProxy		m_uvSet;
	BooleanParamProxy	m_flipU;
	BooleanParamProxy	m_flipV;
	BooleanParamProxy	m_normalize;
	Vec3ParamProxy		m_scale;
	Vec2ParamProxy		m_repeat;

	QVariantMap	m_data;

public:
	UVMappingProxy(std::unique_ptr<IUVMapping> uvMapping, QQmlContext * context);
	~UVMappingProxy();

	void accept();
};
