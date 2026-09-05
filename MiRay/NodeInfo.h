#pragma once

#include "ParamProxy.h"

class INode;
class IMeshNode;
class ILightNode;
class IDirectionalLight;

class NodeInfo : public QObject {
	Q_OBJECT

	StringParamProxy  m_name;
	Vec3ParamProxy    m_position;
	Vec3ParamProxy    m_rotation;
	Vec3ParamProxy    m_scale;

protected:
	QVariantMap m_data;

public:
	NodeInfo(INode & node);

	const QVariantMap & modelData() const { return m_data; }

	static std::unique_ptr<NodeInfo> create(INode & node);
};

class MeshNodeInfo : public NodeInfo {
	Q_OBJECT

	EnumParamProxy    m_renderingLayer;

public:
	MeshNodeInfo(INode & node, IMeshNode * meshNode);
};

class LightNodeInfo : public NodeInfo {
	Q_OBJECT

	ColorParamProxy   m_color;
	ScalarParamProxy  m_intensity;
	ScalarParamProxy  m_radius;

public:
	LightNodeInfo(INode & node, ILightNode * lightNode);
};

class DirectionalLightInfo : public NodeInfo {
	Q_OBJECT

	ColorParamProxy   m_color;
	ScalarParamProxy  m_intensity;
	ScalarParamProxy  m_angularSize;
	ScalarParamProxy  m_yaw;
	ScalarParamProxy  m_pitch;

public:
	DirectionalLightInfo(INode & node, IDirectionalLight * directionalLightNode);
};
