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

#include "Camera.h"
#include "CameraProjector.h"
#include "Tools/FitToView.h"

const mat4 Camera::matCameraSpace(0.f, 1.f, 0.f, 0.f,
								  0.f, 0.f, 1.f, 0.f,
								  1.f, 0.f, 0.f, 0.f,
								  0.f, 0.f, 0.f, 1.f);

static const char * const PROJECTION_TYPES[] = { "Perspective", "Orthographic", "Spherical", "Cylindrical", "Fish Eye", nullptr };

// ------------------------------------------------------------------------ //

Camera::Camera(Scene & scene)
	: m_scene(scene)
	, m_matCamera(1.f)
	, m_matView(1.f)
	, m_matProj(1.f)
	, m_matViewProj(1.f)
	, m_projection(CP_PERSPECTIVE, PROJECTION_TYPES, PID_CAMERA_PROJECTION, *this)
	, m_target(vec3(0.f), 2, PID_CAMERA_TARGET, *this)
	, m_distance(0.f, 0.f, MAX_CAMERA_DISTANCE, 3, PID_CAMERA_DISTANCE, *this)
	, m_yaw(0.f, -360.f, 360.f, 1, PID_CAMERA_YAW, *this)
	, m_pitch(0.f, -89.9f, 89.9f, 1, PID_CAMERA_PITCH, *this)
	, m_roll(0.f, -360.f, 360.f, 1, PID_CAMERA_ROLL, *this)
	, m_fov(40.f, 0.1f, 179.f, 1, PID_CAMERA_FOV, *this)
	, m_nearZ(DEFAULT_ZNEAR, 1e-2f, FLT_MAX, 2, PID_CAMERA_Z_NEAR, *this)
	, m_farZ(DEFAULT_ZFAR, 1.f, FLT_MAX, 1, PID_CAMERA_Z_FAR, *this)
	, m_aspect(1.f, FLT_MIN, FLT_MAX, 3, PID_CAMERA_ASPECT, *this)
	, m_depthOfField(false, PID_CAMERA_DEPTH_OF_FIELD, *this)
	, m_fStop(1.f, FLT_MIN, FLT_MAX, 2, PID_CAMERA_F_STOP, *this)
	, m_focusDistance(1.f, FLT_MIN, FLT_MAX, 3, PID_CAMERA_FOCUS_DISTANCE, *this)
	, m_diaphragmBlades(8, 3, 32, PID_CAMERA_DIAPHRAGM_BLADES, *this)
	, m_bokehRotation(0.f, -180.f, 180.f, 1, PID_CAMERA_BOKEH_ROTATION, *this)
	, m_gamma(1.f, FLT_MIN, FLT_MAX, 3, PID_CAMERA_GAMMA, *this)
{
	CameraState cs;
	cs.reset(vec3(0.f, 0.f, 5.f), 25.f);
	_setState(cs);
}

// ------------------------------------------------------------------------ //

QJsonObject Camera::getState(bool fullInfo) const
{
	return getCameraState().toJson(fullInfo);
}

CameraState Camera::getCameraState() const
{
	CameraState state;
	state.projection = (eCameraProjection)m_projection.getIndex();
	state.target = m_target.get();
	state.distance = m_distance.get();
	state.yaw = m_yaw.get();
	state.pitch = m_pitch.get();
	state.roll = m_roll.get();
	state.fov = m_fov.get();
	state.aspect = m_aspect.get();
	state.nearZ = m_nearZ.get();
	state.farZ = m_farZ.get();
	state.depthOfField = m_depthOfField.get();
	state.fStop = m_fStop.get();
	state.focusDistance = m_focusDistance.get();
	state.diaphragmBlades = m_diaphragmBlades.get();
	state.bokehRotation = m_bokehRotation.get();
	state.gamma = m_gamma.get();
	return state;
}

void Camera::_setState(const QJsonObject & map)
{
	CameraState state;
	state.fromJson(map);
	setState(state);
}

void Camera::setState(const QJsonObject & map)
{
	CameraState state;
	state.fromJson(map);
	setState(state);
}

void Camera::_setState(const CameraState & state)
{
	m_projection._setIndex(state.projection);
	m_target._set(state.target);
	m_distance._set(state.distance);
	m_yaw._set(state.yaw);
	m_pitch._set(state.pitch);
	m_roll._set(state.roll);
	m_fov._set(state.fov);
	m_aspect._set(state.aspect);
	m_nearZ._set(state.nearZ);
	m_farZ._set(state.farZ);
	m_depthOfField._set(state.depthOfField);
	m_fStop._set(state.fStop);
	m_focusDistance._set(state.focusDistance);
	m_diaphragmBlades._set(state.diaphragmBlades);
	m_bokehRotation._set(state.bokehRotation);
	m_gamma._set(state.gamma);

	if (!m_projector.get() || static_cast<int>(m_projector->type()) != m_projection.getIndex())
		m_projector.reset(CreateCameraProjector((eCameraProjection)m_projection.getIndex()));

	fireChanged(PID_CAMERA);
}

void Camera::setState(const CameraState & state)
{
	m_scene.lock(SceneInternalModification_Camera);
	_setState(state);
	m_scene.unlock(SceneInternalModification_Camera);
}

// ------------------------------------------------------------------------ //

void Camera::setAnimationState(const QJsonObject & state1, const QJsonObject & state2, uint32_t flags)
{
	m_animState[0] = m_animState[1] = getCameraState();
	if ((flags & SF_HAS_CAMERA_STATE) && !state1.isEmpty() && !state2.isEmpty()) {
		m_animState[0].fromJson(state1);
		m_animState[1].fromJson(state2);
	}
}

void Camera::setAnimationTime(float t)
{
	m_projection._setIndex(t < 0.5f ? m_animState[0].projection : m_animState[1].projection);
	m_target._set(glm::mix(m_animState[0].target, m_animState[1].target, t));
	m_distance._set(lerp(m_animState[0].distance, m_animState[1].distance, t));

	const glm::quat q1(MatrixBuilder().AddRotation(vec3(m_animState[0].roll, m_animState[0].pitch, m_animState[0].yaw)));
	const glm::quat q2(MatrixBuilder().AddRotation(vec3(m_animState[1].roll, m_animState[1].pitch, m_animState[1].yaw)));
	const auto q = glm::slerp(q1, q2, t);
	auto rotation = glm::getRotation(glm::toMat4(q));
	m_yaw._set(rotation.z);
	m_pitch._set(rotation.y);
	m_roll._set(rotation.x);

	m_fov._set(lerp(m_animState[0].fov, m_animState[1].fov, t));
	m_aspect._set(lerp(m_animState[0].aspect, m_animState[1].aspect, t));
	m_nearZ._set(lerp(m_animState[0].nearZ, m_animState[1].nearZ, t));
	m_farZ._set(lerp(m_animState[0].farZ, m_animState[1].farZ, t));
	m_depthOfField._set(t < 0.5f ? m_animState[0].depthOfField : m_animState[1].depthOfField);
	m_fStop._set(lerp(m_animState[0].fStop, m_animState[1].fStop, t));
	m_focusDistance._set(lerp(m_animState[0].focusDistance, m_animState[1].focusDistance, t));
	m_diaphragmBlades._set((int)lerp<float>(m_animState[0].diaphragmBlades, m_animState[1].diaphragmBlades, t));
	m_bokehRotation._set(lerp(m_animState[0].bokehRotation, m_animState[1].bokehRotation, t));
	m_gamma._set(lerp(m_animState[0].gamma, m_animState[1].gamma, t));

	if (!m_projector.get() || static_cast<int>(m_projector->type()) != m_projection.getIndex())
		m_projector.reset(CreateCameraProjector((eCameraProjection)m_projection.getIndex()));

	updateCameraMatrix();
	updateProjMatrix();
//	updateMatrices();
	m_matView = glm::inverse(m_matCamera);
	m_matViewProj = m_matProj * m_matView;

	if (m_projector.get())
		m_projector->update(*this);
}

// ------------------------------------------------------------------------ //

void CameraState::reset(const vec3 & pos, float dist)
{
	projection = CP_PERSPECTIVE;
	target = pos;
	distance = std::clamp(dist, 0.f, MAX_CAMERA_DISTANCE);
	yaw = 0.f;
	pitch = 10.f;
	roll = 0.f;
	fov = 40.f;
	aspect = 4.f / 3.f;
	farZ = std::max(DEFAULT_ZFAR, dist * DEFAULT_ZFAR_CAM_DIST_RATIO);
	nearZ = std::max(DEFAULT_ZNEAR, farZ * MIN_ZNEAR_ZFAR_RATIO);
	depthOfField = false;
	focusDistance = dist;
	diaphragmBlades = 32; // circular
	bokehRotation = 0.f;
	fStop = 4.f;
	gamma = 1.f;
}

QJsonObject CameraState::toJson(bool fullInfo) const
{
	QJsonObject state {
		{ "flags", depthOfField ? 8 : 0 },
		{ "projection", (int)projection },
		{ "target", toJsonArray(target) },
		{ "rotation", toJsonArray(vec3(roll, pitch, yaw)) },
		{ "dist", distance },
		{ "fov", fov },
		{ "aspect", aspect },
		{ "nearZ", nearZ },
		{ "farZ", farZ }
	};
	if (fullInfo || depthOfField) {
		state["depthOfField"] = QJsonObject {
			{ "fStop", fStop },
			{ "focusDistance", focusDistance },
			{ "diaphragmBlades", diaphragmBlades },
			{ "bokehRotation", bokehRotation },
			{ "gamma", gamma }
		};
	}
	return state;
}

void CameraState::fromJson(const QJsonObject & state)
{
	int flags = state.value("flags").toInt(1 | 2 | 4);
	projection = (eCameraProjection)state.value("projection").toInt(CP_PERSPECTIVE);
	target = getVec3(state, "target", vec3(0.f));
	auto rotation = getVec3(state, "rotation", vec3(0.f));
	yaw = rotation.z;
	pitch = rotation.y;
	roll = rotation.x;
	distance = state.value("dist").toDouble(50.f);
	fov = state.value("fov").toDouble(42.f);
	aspect = state.value("aspect").toDouble(16.f / 9.f);
	nearZ = state.value("nearZ").toDouble(DEFAULT_ZNEAR);
	farZ = state.value("farZ").toDouble(DEFAULT_ZFAR);
	depthOfField = flags & 8;
	QString dofKey("depthOfField");
	auto it = state.find(dofKey);
	if (it != state.end()) {
		QJsonObject dofState = it.value().toObject();
		fStop = dofState.value("fStop").toDouble(1.f);
		focusDistance = dofState.value("focusDistance").toDouble(100.f);
		diaphragmBlades = dofState.value("diaphragmBlades").toInt(8);
		bokehRotation = dofState.value("bokehRotation").toDouble(0.f);
		gamma = dofState.value("gamma").toDouble(1.2f);
	} else {
		fStop = 1.f;
		focusDistance = 100.f;
		diaphragmBlades = 8;
		bokehRotation = 0.f;
		gamma = 1.2f;
	}
}

// ------------------------------------------------------------------------ //

void Camera::reset()
{
	auto & bbox = m_scene.root().oobb();
	vec3 target;
	float dist;
	if (bbox.isNull()) {
		target = vec3(0.f, 0.f, 5.f);
		dist = 25.f;
	} else {
		target = bbox.center();
		dist = std::max(glm::length(bbox.size()) * 1.5f, 1.f);
	}

	CameraState state;
	state.reset(target, dist);
	_setState(state);
}

void Camera::update()
{
	_setState(getCameraState());
}

// ------------------------------------------------------------------------ //

bool Camera::canFitToView() const
{
	return m_projection.getIndex() == CP_PERSPECTIVE || m_projection.getIndex() == CP_ORTHOGRAPHIC;
}

std::unique_ptr<IFitToView> Camera::beginFitToView()
{
	return std::unique_ptr<IFitToView>(new FitToView(m_scene));
}

// ------------------------------------------------------------------------ //

void Camera::rotate(float dYaw, float dPitch)
{
	m_scene.lock(SceneInternalModification_Camera);
	auto pitch = std::clamp(m_pitch.get() + dPitch, m_pitch.min(), m_pitch.max());
	m_pitch._set(pitch);
	m_yaw._set(std::fmod(m_yaw.get() + dYaw, 360.f));
	fireChanged(PID_CAMERA_YAW);
	m_scene.unlock(SceneInternalModification_Camera);
}

void Camera::pan(float dx, float dy, const vec3 * target)
{
	m_scene.lock(SceneInternalModification_Camera);
	float dist = m_projection.getIndex() != CP_ORTHOGRAPHIC ?
		(target ? glm::dot(glm::axisZ(m_matCamera), (glm::translation(m_matCamera) - *target)) : m_distance.get()) : 1;
	auto deltaX = glm::axisX(m_matCamera) * (-dx * dist / m_matProj[0][0]);
	auto deltaY = glm::axisY(m_matCamera) * (dy * dist / m_matProj[1][1]);
	m_target._set(m_target.get() + deltaX + deltaY);
	fireChanged(PID_CAMERA_TARGET);
	m_scene.unlock(SceneInternalModification_Camera);
}

void Camera::zoom(float delta)
{
	m_scene.lock(SceneInternalModification_Camera);
	const float zoom = std::pow(1.03f, std::fabs(delta));
	float dist = delta < 0 ? m_distance.get() / zoom : m_distance.get() * zoom;
	if (std::fabs(dist - m_distance.get()) < std::fabs(delta * 0.5f))
		dist = m_distance.get() + delta * 0.5f;
	dist = std::clamp(dist, 0.f, MAX_CAMERA_DISTANCE);
	m_focusDistance._set(m_focusDistance.get() + dist - m_distance.get());
	m_distance._set(dist);
	fireChanged(PID_CAMERA_DISTANCE);
	m_scene.unlock(SceneInternalModification_Camera);
}

void Camera::setTarget(const vec3 & target)
{
	auto oldState = getCameraState();
	auto newState = oldState;
	auto dir = glm::translation(m_matCamera) - target;
	newState.yaw = glm::degrees(getYaw(dir));
	newState.pitch = glm::degrees(getPitch(dir));
	newState.focusDistance = newState.distance = glm::length(dir);
	newState.target = target;
	cameraChangeState(*this, oldState, newState);
}

void Camera::setFocus(const vec3 & target)
{
	m_focusDistance.set(-glm::transformCoord(target, m_matView).z);
}

//void MatrixSetOrtho(mat4 & m, float left, float right, float bottom, float top, float znear, float zfar)
//{
//	m.m11 = 2.f / (right - left);
//	m.m12 = 0.f;
//	m.m13 = 0.f;
//	m.m14 = 0.f;
//
//	m.m21 = 0.f;
//	m.m22 = 2.f / (top - bottom);
//	m.m23 = 0.f;
//	m.m24 = 0.f;
//
//	m.m31 = 0.f;
//	m.m32 = 0.f;
//	m.m33 = -2.f / (zfar - znear);
//	m.m34 = 0.f;
//
//	m.m41 = -(right + left) / (right - left);
//	m.m42 = -(top + bottom) / (top - bottom);
//	m.m43 = -(zfar + znear) / (zfar - znear);
//	m.m44 = 1.f;
//}

void Camera::updateProjMatrix(float aspect)
{
	if (m_projection.getIndex() != CP_ORTHOGRAPHIC) {
		float dy = m_nearZ.get() * std::tan(glm::radians(m_fov.get()) * 0.5f);
		float dx = dy * aspect;
		m_matProj = glm::frustumRH_NO(-dx, dx, -dy, dy, m_nearZ.get(), m_farZ.get());
	} else {
		float dy = m_distance.get() * std::tan(glm::radians(m_fov.get()) * 0.5f);
		float dx = dy * aspect;
//		MatrixSetOrtho(m_matProj, -dx, dx, -dy, dy, m_nearZ.get(), m_farZ.get());
		m_matProj = glm::orthoRH_NO(-dx, dx, -dy, dy, m_nearZ.get(), m_farZ.get());
	}
}

void Camera::updateProjMatrix()
{
	updateProjMatrix(m_aspect.get());
}

void Camera::updateCameraMatrix()
{
	auto rotation = vec3(m_roll.get(), m_pitch.get(), m_yaw.get());
	auto pos = m_target.get() + anglesToVector(rotation) * m_distance.get();
	m_matCamera = mat4(MatrixBuilder().AddPosition(pos).AddRotation(rotation)) * matCameraSpace;
}

void Camera::updateMatrices()
{
	m_matView = glm::inverse(m_matCamera);
	m_matViewProj = m_matProj * m_matView;

	if (m_projector.get())
		m_projector->update(*this);

	m_scene.fireCameraChanged();
}

void Camera::setRenderAspect(float aspect)
{
	updateProjMatrix(aspect);
	m_matView = glm::inverse(m_matCamera);
	m_matViewProj = m_matProj * m_matView;

	if (m_projector.get())
		m_projector->update(*this);
}

// ------------------------------------------------------------------------ //

CoreInstance & Camera::core() const
{
	return m_scene.core();
}

void Camera::pushCommand(QUndoCommand *cmd)
{
	m_scene.pushCommand(cmd);
}

void Camera::fireChanged(eParamId paramId)
{
	switch (paramId) {
		case PID_CAMERA_Z_NEAR:
		case PID_CAMERA_Z_FAR: {
			if (m_farZ.get() < m_nearZ.get() + MIN_ZNEAR_ZFAR_DISTANCE)
				m_farZ._set(m_nearZ.get() + MIN_ZNEAR_ZFAR_DISTANCE);
			else if (m_nearZ.get() < m_farZ.get() * MIN_ZNEAR_ZFAR_RATIO)
				m_nearZ._set(m_farZ.get() * MIN_ZNEAR_ZFAR_RATIO);
			break;
		}

		case PID_CAMERA_PROJECTION:
			if (!m_projector.get() || static_cast<int>(m_projector->type()) != m_projection.getIndex()) {
				m_scene.lock(SceneInternalModification_Camera);
				m_projector.reset(CreateCameraProjector((eCameraProjection)m_projection.getIndex()));
				m_scene.unlock(SceneInternalModification_Camera);
				m_depthOfField.setEnabled(m_projection.getIndex() == CP_PERSPECTIVE);
			}
		case PID_CAMERA:
		case PID_CAMERA_DEPTH_OF_FIELD: {
			auto depthOfField = m_depthOfField.get() && m_depthOfField.isEnabled();
			m_fStop.setEnabled(depthOfField);
			m_focusDistance.setEnabled(depthOfField);
			m_diaphragmBlades.setEnabled(depthOfField);
			m_bokehRotation.setEnabled(depthOfField && m_diaphragmBlades.get() > 2);

			m_fStop.setVisible(depthOfField);
			m_focusDistance.setVisible(depthOfField);
			m_diaphragmBlades.setVisible(depthOfField);
			m_bokehRotation.setVisible(depthOfField);
			break;
		}

		case PID_CAMERA_DIAPHRAGM_BLADES:
			m_bokehRotation.setEnabled(m_depthOfField.get() && m_diaphragmBlades.get() > 2);
			break;
	}

	updateCameraMatrix();
	updateProjMatrix();
	updateMatrices();
}
