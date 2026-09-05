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

#include "SnapshotImpl.h"
#include "Utils.h"

SnapshotImpl::SnapshotImpl(Scene & scene, const QString & name)
	: m_scene(scene)
	, m_guid(scene.createGuid())
	, m_name(name, PID_SNAPSHOT_NAME, *this, nullptr)
	, m_hasCameraState(true, PID_SNAPSHOT_HAS_CAMERA_STATE, *this)
	, m_hasVisibility(true, PID_SNAPSHOT_HAS_VISIBILITY, *this)
	, m_hasTransformations(true, PID_SNAPSHOT_HAS_TRANSFORMATIONS, *this)
	, m_hasAssignedMaterials(true, PID_SNAPSHOT_HAS_ASSIGNED_MATERIALS, *this)
	, m_hasEnvironment(true, PID_SNAPSHOT_HAS_ENVIRONMENT, *this)
	, m_hasBackground(true, PID_SNAPSHOT_HAS_BACKGROUND, *this)
{
}

SnapshotImpl::~SnapshotImpl()
{
}

// ------------------------------------------------------------------------ //

void SnapshotImpl::activate() const
{
	m_scene.setState(m_state, getFlags());
}

uint32_t SnapshotImpl::getFlags() const
{
	uint32_t flags = 0;
	if (m_hasCameraState.get())			flags |= SF_HAS_CAMERA_STATE;
	if (m_hasVisibility.get())			flags |= SF_HAS_VISIBILITY;
	if (m_hasTransformations.get())		flags |= SF_HAS_TRANSFORMATIONS;
	if (m_hasAssignedMaterials.get())	flags |= SF_HAS_ASSIGNED_MATERIALS;
	if (m_hasEnvironment.get())			flags |= SF_HAS_ENVIRONMENT;
	if (m_hasBackground.get())			flags |= SF_HAS_BACKGROUND;
	return flags;
}

SnapshotParams	SnapshotImpl::getParameters() const
{
	SnapshotParams params;
	params.name = m_name.get();

	params.isCameraValid = m_state.contains("camera");

	if (params.isCameraValid) {
		QJsonObject state = m_state["camera"].toObject();
		SnapshotCameraParams &cam = params.camera;

		int flags = state.value("flags").toInt(1 | 2 | 4);
		cam.center = getVec3(state, "target", vec3(0.f));
		auto rotation = getVec3(state, "rotation", vec3(0.f));
		cam.yaw = rotation.z;
		cam.pitch = rotation.y;
		cam.roll = rotation.x;
		cam.distance = state.value("dist").toDouble(50.f);
		cam.fov = state.value("fov").toDouble(42.f);
		cam.aspect = state.value("aspect").toDouble(16.f / 9.f);
		cam.nearZ = state.value("nearZ").toDouble(DEFAULT_ZNEAR);
		cam.farZ = state.value("farZ").toDouble(DEFAULT_ZFAR);
		cam.depthOfField = (flags & 8) != 0;

		QString dofKey("depthOfField");
		if (state.contains(dofKey)) {
			QJsonObject dofState = state[dofKey].toObject();

			cam.fStop = dofState.value("fStop").toDouble(1.f);
			cam.focusDistance = dofState.value("focusDistance").toDouble(100.f);
		}

	}

	return params;
}

// ------------------------------------------------------------------------ //

class UpdateSnapshotCommand : public QUndoCommand
{
	SnapshotImpl &	m_snapshot;
	QJsonObject		m_oldState, m_newState;
	QImage			m_oldPreview, m_newPreview;

	void redo() override
	{
		m_snapshot._setState(m_newState);
		m_snapshot._setPreview(m_newPreview);
		m_snapshot.fireChanged(PID_SNAPSHOT);
	}

	void undo() override
	{
		m_snapshot._setState(m_oldState);
		m_snapshot._setPreview(m_oldPreview);
		m_snapshot.fireChanged(PID_SNAPSHOT);
	}

public:
	UpdateSnapshotCommand(SnapshotImpl & cs, QJsonObject state, QImage preview)
		: m_snapshot(cs)
		, m_oldState(cs.getState())
		, m_newState(state)
		, m_oldPreview(cs.getPreview())
		, m_newPreview(preview)
	{
	}
};

void SnapshotImpl::updateWithCurrent()
{
	m_scene.pushCommand(new UpdateSnapshotCommand(*this, m_scene.getState(true), m_scene.getPreview()));
}

// ------------------------------------------------------------------------ //

CoreInstance & SnapshotImpl::core() const
{
	return m_scene.core();
}

void SnapshotImpl::pushCommand(QUndoCommand *cmd)
{
	m_scene.pushCommand(cmd);
}

void SnapshotImpl::fireChanged(eParamId paramId)
{
	m_scene.snapshotManager().fireStateChanged(this);
}
