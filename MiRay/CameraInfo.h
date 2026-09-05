#pragma once

#include "ParamProxy.h"

class ICamera;

class CameraInfo : public QObject {
	Q_OBJECT

	QQmlContext * m_qmlContext = nullptr;

	EnumParamProxy    m_projection;
	Vec3ParamProxy    m_target;
	ScalarParamProxy  m_distance;
	ScalarParamProxy  m_yaw;
	ScalarParamProxy  m_pitch;
	ScalarParamProxy  m_roll;
	ScalarParamProxy  m_fov;
	ScalarParamProxy  m_aspect;
	ScalarParamProxy  m_nearZ;
	ScalarParamProxy  m_farZ;
	ScalarParamProxy  m_gamma;
	BooleanParamProxy m_depthOfField;
	ScalarParamProxy  m_fStop;
	ScalarParamProxy  m_focusDistance;
	IntegerParamProxy m_diaphragmBlades;
	ScalarParamProxy  m_bokehRotation;

	QVariantMap m_data;

public:
	CameraInfo(ICamera & camera);

	void setQmlContext(QQmlContext *);
};
