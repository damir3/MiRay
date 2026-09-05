/*
	Copyright (C) 2013-2020 Damir Sagidullin

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
	documentation files (the "Software"), to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions
	of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
	TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
	THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
	DEALINGS IN THE SOFTWARE.

	(The above is MIT License: http://en.wikipedia.origin/wiki/MIT_License)
*/

#pragma once

#include "ParametersImpl.h"

#define MAX_CAMERA_DISTANCE				1e+6f	// 10 km
#define DEFAULT_ZNEAR					0.1f	// 0.1 cm
#define DEFAULT_ZFAR					1000.f	// 10 m
#define MIN_ZNEAR_ZFAR_DISTANCE			0.1f	// 0.1 cm
#define MIN_ZNEAR_ZFAR_RATIO			1e-5f
#define DEFAULT_ZFAR_CAM_DIST_RATIO		5.f

struct CameraState {
	eCameraProjection	projection;
	vec3	target;
	float	distance;
	float	yaw;
	float	pitch;
	float	roll;
	float	fov;
	float	aspect;
	float	nearZ;
	float	farZ;
	bool	depthOfField;
	float	fStop;
	int		diaphragmBlades;
	float	bokehRotation;
	float	focusDistance;
	float	gamma;

	void reset(const vec3 & target, float dist);
	QJsonObject toJson(bool fullInfo = true) const;
	void fromJson(const QJsonObject & map);
};

class Camera final : public ICamera, public IParameterOwner
{
	class Scene & m_scene;
	mat4	m_matCamera;
	mat4	m_matView;
	mat4	m_matProj;
	mat4	m_matViewProj;
	EnumParameterImpl		m_projection;
	Vec3ParameterImpl		m_target;
	ScalarParameterImpl		m_distance;
	ScalarParameterImpl		m_yaw;
	ScalarParameterImpl		m_pitch;
	ScalarParameterImpl		m_roll;
	ScalarParameterImpl		m_fov;
	ScalarParameterImpl		m_aspect;
	ScalarParameterImpl		m_nearZ;
	ScalarParameterImpl		m_farZ;
	BooleanParameterImpl	m_depthOfField;
	ScalarParameterImpl		m_fStop;
	ScalarParameterImpl		m_focusDistance;
	IntegerParameterImpl	m_diaphragmBlades;
	ScalarParameterImpl		m_bokehRotation;
	ScalarParameterImpl		m_gamma;

	std::unique_ptr<ICameraProjector>	m_projector;

	CameraState			m_animState[2];

	void updateProjMatrix(float aspect);

public:
	Camera(Scene & scene);

	QJsonObject getState(bool fullInfo = true) const;
	void _setState(const QJsonObject & state);
	void setState(const QJsonObject & state);

	CameraState getCameraState() const;
	void setState(const CameraState & state);
	void _setState(const CameraState & state);

	void setAnimationState(const QJsonObject & state1, const QJsonObject & state2, uint32_t flags);
	void setAnimationTime(float t);

	void reset();
	void update() override;

	EnumParameterImpl & projection() override { return m_projection; }
	Vec3ParameterImpl & target() override { return m_target; }
	ScalarParameterImpl & distance() override { return m_distance; }
	ScalarParameterImpl & yaw() override { return m_yaw; }
	ScalarParameterImpl & pitch() override { return m_pitch; }
	ScalarParameterImpl & roll() override { return m_roll; }
	ScalarParameterImpl & fov() override { return m_fov; }
	ScalarParameterImpl & aspect() override { return m_aspect; }
	ScalarParameterImpl & nearZ() override { return m_nearZ; }
	ScalarParameterImpl & farZ() override { return m_farZ; }
	BooleanParameterImpl & depthOfField() override { return m_depthOfField; }
	ScalarParameterImpl & fStop() override { return m_fStop; }
	ScalarParameterImpl & focusDistance() override { return m_focusDistance; }
	IntegerParameterImpl & diaphragmBlades() override { return m_diaphragmBlades; }
	ScalarParameterImpl & bokehRotation() override { return m_bokehRotation; }
	ScalarParameterImpl & gamma() override { return m_gamma; }

	void updateCameraMatrix();
	void updateProjMatrix();
	void updateMatrices();
	void setRenderAspect(float aspect);

	bool canFitToView() const override;
	std::unique_ptr<IFitToView> beginFitToView() override;

	void rotate(float dYaw, float dPitch);
	void pan(float dx, float dy, const vec3 * target);
	void zoom(float delta);
	void setTarget(const vec3 & target);
	void setFocus(const vec3 & target);

	const mat4 & matrix() const { return m_matCamera; }
	const mat4 & viewMatrix() const { return m_matView; }
	const mat4 & projMatrix() const { return m_matProj; }
	const mat4 & viewProjMatrix() const { return m_matViewProj; }

	ICameraProjector * rayProjector() const override { return m_projector.get(); }

	CoreInstance & core() const override;
	void pushCommand(QUndoCommand *cmd) override;
	void fireChanged(eParamId paramId) override;

	static const mat4	matCameraSpace;
	static const mat4	matCameraSpaceInv;
};
