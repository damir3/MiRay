#include "NodeInfo.h"

#include "../Shared/Interfaces/Node.h"
#include "../Shared/Interfaces/MeshNode.h"
#include "../Shared/Interfaces/LightNode.h"

NodeInfo::NodeInfo(INode & node)
	: m_name("Name")
	, m_position("Position")
	, m_rotation("Rotation")
	, m_scale("Scale") {
	m_name.setParam(&node.name());
	m_position.setParam(&node.position());
	m_rotation.setParam(&node.rotation());
	m_scale.setParam(&node.scale());

	m_data["propTitle"] = "";
	m_data["name"] = QVariant::fromValue(static_cast<QObject *>(&m_name));
	m_data["position"] = QVariant::fromValue(static_cast<QObject *>(&m_position));
	m_data["rotation"] = QVariant::fromValue(static_cast<QObject *>(&m_rotation));
	m_data["scale"] = QVariant::fromValue(static_cast<QObject *>(&m_scale));
}

std::unique_ptr<NodeInfo> NodeInfo::create(INode & node) {
	switch (node.type()) {
	case SceneElement_MeshNode:
		return std::make_unique<MeshNodeInfo>(node, qobject_cast<IMeshNode *>(&node));
	case SceneElement_Light:
		return std::make_unique<LightNodeInfo>(node, qobject_cast<ILightNode *>(&node));
	case SceneElement_DirectionalLight:
		return std::make_unique<DirectionalLightInfo>(node, qobject_cast<IDirectionalLight *>(&node));
	default:
		return std::make_unique<NodeInfo>(node);
	}
}

MeshNodeInfo::MeshNodeInfo(INode & node, IMeshNode * meshNode)
	: NodeInfo(node)
	, m_renderingLayer("Rendering Layer") {
	if (meshNode) {
		m_renderingLayer.setParam(&meshNode->renderLayer());
	}
	m_data["meshRenderingLayer"] = QVariant::fromValue(static_cast<QObject *>(&m_renderingLayer));
}

LightNodeInfo::LightNodeInfo(INode & node, ILightNode * lightNode)
	: NodeInfo(node)
	, m_color("Color")
	, m_intensity("Intensity")
	, m_radius("Radius") {
	if (lightNode) {
		m_color.setParam(&lightNode->color());
		m_intensity.setParam(&lightNode->intensity());
		m_radius.setParam(&lightNode->radius());
	}
	m_data["lightColor"] = QVariant::fromValue(static_cast<QObject *>(&m_color));
	m_data["lightIntensity"] = QVariant::fromValue(static_cast<QObject *>(&m_intensity));
	m_data["lightRadius"] = QVariant::fromValue(static_cast<QObject *>(&m_radius));
}

DirectionalLightInfo::DirectionalLightInfo(INode & node, IDirectionalLight * directionalLightNode)
	: NodeInfo(node)
	, m_color("Color")
	, m_intensity("Intensity")
	, m_angularSize("Angular size")
	, m_yaw("Yaw")
	, m_pitch("Pitch") {
	if (directionalLightNode) {
		m_color.setParam(&directionalLightNode->color());
		m_intensity.setParam(&directionalLightNode->intensity());
		m_angularSize.setParam(&directionalLightNode->angularSize());
		m_yaw.setParam(&directionalLightNode->yaw());
		m_pitch.setParam(&directionalLightNode->pitch());
	}
	m_data["lightColor"] = QVariant::fromValue(static_cast<QObject *>(&m_color));
	m_data["lightIntensity"] = QVariant::fromValue(static_cast<QObject *>(&m_intensity));
	m_data["lightAngularSize"] = QVariant::fromValue(static_cast<QObject *>(&m_angularSize));
	m_data["lightYaw"] = QVariant::fromValue(static_cast<QObject *>(&m_yaw));
	m_data["lightPitch"] = QVariant::fromValue(static_cast<QObject *>(&m_pitch));
}
