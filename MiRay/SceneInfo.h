#pragma once

#include "ParamProxy.h"

class ISceneProperties;

class SceneInfo : public QObject {
	Q_OBJECT

	QQmlContext * m_qmlContext = nullptr;

	EnumParamProxy    m_bgMode;
	ColorParamProxy   m_bgColor;
	ColorParamProxy   m_bgColor2;

	ColorParamProxy   m_envColor;
	ScalarParamProxy  m_envIntensity;
	ScalarParamProxy  m_envSize;
	ScalarParamProxy  m_envVerticalOffset;
	ScalarParamProxy  m_envHorizontalRotation;
	ScalarParamProxy  m_envVerticalRotation;
	ScalarParamProxy  m_diffuseIntensity;

	BooleanParamProxy m_floorEnabled;
	ScalarParamProxy  m_floorReflectionLevel;
	ScalarParamProxy  m_floorRoughness;
	ScalarParamProxy  m_floorShadowLevel;

	QVariantMap m_data;

public:
	SceneInfo(ISceneProperties & scene);

	void setQmlContext(QQmlContext *);
};
