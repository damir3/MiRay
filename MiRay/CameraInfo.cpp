#include "CameraInfo.h"

#include "../Shared/Interfaces/Camera.h"

CameraInfo::CameraInfo(ICamera & camera)
	: m_projection("Projection")
	, m_target("Target")
	, m_distance("Distance")
	, m_yaw("Yaw")
	, m_pitch("Pitch")
	, m_roll("Roll")
	, m_fov("Field of view")
	, m_aspect("Aspect")
	, m_nearZ("Z near")
	, m_farZ("Z far")
	, m_gamma("Gamma")
	, m_depthOfField("Depth of field")
	, m_fStop("f-Stop")
	, m_focusDistance("Focus distance")
	, m_diaphragmBlades("Diaphragm blades")
	, m_bokehRotation("Bokeh rotation") {
	m_projection.setParam(&camera.projection());
	m_target.setParam(&camera.target());
	m_distance.setParam(&camera.distance());
	m_yaw.setParam(&camera.yaw());
	m_pitch.setParam(&camera.pitch());
	m_roll.setParam(&camera.roll());
	m_fov.setParam(&camera.fov());
	m_aspect.setParam(&camera.aspect());
	m_nearZ.setParam(&camera.nearZ());
	m_farZ.setParam(&camera.farZ());
	m_gamma.setParam(&camera.gamma());
	m_depthOfField.setParam(&camera.depthOfField());
	m_fStop.setParam(&camera.fStop());
	m_focusDistance.setParam(&camera.focusDistance());
	m_diaphragmBlades.setParam(&camera.diaphragmBlades());
	m_bokehRotation.setParam(&camera.bokehRotation());

	m_data["projection"] = QVariant::fromValue(static_cast<QObject *>(&m_projection));
	m_data["target"] = QVariant::fromValue(static_cast<QObject *>(&m_target));
	m_data["distance"] = QVariant::fromValue(static_cast<QObject *>(&m_distance));
	m_data["yaw"] = QVariant::fromValue(static_cast<QObject *>(&m_yaw));
	m_data["pitch"] = QVariant::fromValue(static_cast<QObject *>(&m_pitch));
	m_data["roll"] = QVariant::fromValue(static_cast<QObject *>(&m_roll));
	m_data["fov"] = QVariant::fromValue(static_cast<QObject *>(&m_fov));
	m_data["aspect"] = QVariant::fromValue(static_cast<QObject *>(&m_aspect));
	m_data["nearZ"] = QVariant::fromValue(static_cast<QObject *>(&m_nearZ));
	m_data["farZ"] = QVariant::fromValue(static_cast<QObject *>(&m_farZ));
	m_data["gamma"] = QVariant::fromValue(static_cast<QObject *>(&m_gamma));
	m_data["depthOfField"] = QVariant::fromValue(static_cast<QObject *>(&m_depthOfField));
	m_data["fStop"] = QVariant::fromValue(static_cast<QObject *>(&m_fStop));
	m_data["focusDistance"] = QVariant::fromValue(static_cast<QObject *>(&m_focusDistance));
	m_data["diaphragmBlades"] = QVariant::fromValue(static_cast<QObject *>(&m_diaphragmBlades));
	m_data["bokehRotation"] = QVariant::fromValue(static_cast<QObject *>(&m_bokehRotation));
}

void CameraInfo::setQmlContext(QQmlContext * qmlContext)
{
	if (m_qmlContext == qmlContext)
		return;

	if (m_qmlContext)
		m_qmlContext->setContextProperty("cameraModel", nullptr);

	m_qmlContext = qmlContext;

	if (m_qmlContext)
		m_qmlContext->setContextProperty("cameraModel", m_data);
}
