#include "SceneInfo.h"

#include "../Shared/Interfaces/Scene.h"

SceneInfo::SceneInfo(ISceneProperties & scene)
	: m_qmlContext(nullptr)
	, m_bgMode("Mode")
	, m_bgColor("Color", "Background Map")
	, m_bgColor2("Gradient color")
	, m_envColor("Color", "Environment Map")
	, m_envIntensity("Intensity")
	, m_envSize("Size")
	, m_envVerticalOffset("Vertical offset")
	, m_envHorizontalRotation("Horizontal rotation")
	, m_envVerticalRotation("Vertical rotation")
	, m_diffuseIntensity("Diffuse Intensity")
	, m_floorEnabled("Show floor")
	, m_floorReflectionLevel("Reflection level")
	, m_floorRoughness("Roughness")
	, m_floorShadowLevel("Shadow level") {
	m_bgMode.setParam(&scene.backgroundMode());
	m_bgColor.setParam(&scene.background());
	m_bgColor2.setParam(&scene.backgroundColor2());
	m_envColor.setParam(&scene.environment());
	m_envIntensity.setParam(&scene.environmentIntensity());
	m_envSize.setParam(&scene.environmentSize());
	m_envVerticalOffset.setParam(&scene.environmentVerticalOffset());
	m_envHorizontalRotation.setParam(&scene.environmentHorizontalRotation());
	m_envVerticalRotation.setParam(&scene.environmentVerticalRotation());
	m_diffuseIntensity.setParam(&scene.diffuseIntensity());
	m_floorEnabled.setParam(&scene.floorEnabled());
	m_floorReflectionLevel.setParam(&scene.floorReflectionLevel());
	m_floorRoughness.setParam(&scene.floorRoughness());
	m_floorShadowLevel.setParam(&scene.floorShadowLevel());

	m_data["bgMode"] = QVariant::fromValue(static_cast<QObject *>(&m_bgMode));
	m_data["bgColor"] = QVariant::fromValue(static_cast<QObject *>(&m_bgColor));
	m_data["bgColor2"] = QVariant::fromValue(static_cast<QObject *>(&m_bgColor2));
	m_data["envColor"] = QVariant::fromValue(static_cast<QObject *>(&m_envColor));
	m_data["envIntensity"] = QVariant::fromValue(static_cast<QObject *>(&m_envIntensity));
	m_data["envSize"] = QVariant::fromValue(static_cast<QObject *>(&m_envSize));
	m_data["envVerticalOffset"] = QVariant::fromValue(static_cast<QObject *>(&m_envVerticalOffset));
	m_data["envHorizontalRotation"] = QVariant::fromValue(static_cast<QObject *>(&m_envHorizontalRotation));
	m_data["envVerticalRotation"] = QVariant::fromValue(static_cast<QObject *>(&m_envVerticalRotation));
	m_data["diffuseIntensity"] = QVariant::fromValue(static_cast<QObject *>(&m_diffuseIntensity));
	m_data["floorEnabled"] = QVariant::fromValue(static_cast<QObject *>(&m_floorEnabled));
	m_data["floorReflectionLevel"] = QVariant::fromValue(static_cast<QObject *>(&m_floorReflectionLevel));
	m_data["floorRoughness"] = QVariant::fromValue(static_cast<QObject *>(&m_floorRoughness));
	m_data["floorShadowLevel"] = QVariant::fromValue(static_cast<QObject *>(&m_floorShadowLevel));
}

void SceneInfo::setQmlContext(QQmlContext * qmlContext)
{
	if (m_qmlContext == qmlContext)
		return;

	if (m_qmlContext)
		m_qmlContext->setContextProperty("sceneModel", nullptr);

	m_qmlContext = qmlContext;

	if (m_qmlContext)
		m_qmlContext->setContextProperty("sceneModel", m_data);
}
