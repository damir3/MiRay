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

#include "SnapshotInterpolator.h"

enum { DEFAULT_FRAME_COUNT = 25 };

SnapshotInterpolator::SnapshotInterpolator(Scene & scene, const ISnapshot & snapshot1, const ISnapshot & snapshot2)
	: m_scene(scene)
	, m_defaultState(scene.getState(true))
	, m_transitionFrame(DEFAULT_FRAME_COUNT/2, 0, DEFAULT_FRAME_COUNT, PID_SNAPSHOT_INTERPOLATOR_TRANSITION_FRAME, *this)
	, m_transitionFrameCount(DEFAULT_FRAME_COUNT, 1, INT_MAX, PID_SNAPSHOT_INTERPOLATOR_TRANSITION_FRAME_COUNT, *this)
	, m_transitionExponent(2.f, 0.001f, 1000.f, 3, PID_SNAPSHOT_INTERPOLATOR_TRANSITION_EXPONENT, *this)
	, m_motionBlur(100.f, 0.f, 100.f, 1, PID_SNAPSHOT_INTERPOLATOR_TRANSITION_MOTION_BLUR, *this)
{
	m_scene.storeTransitionState(2);
	auto & s1 = static_cast<const SnapshotImpl &>(snapshot1);
	auto & s2 = static_cast<const SnapshotImpl &>(snapshot2);
	m_defaultStateFlags = s1.getFlags() | s2.getFlags();
	m_scene._setState(s1.getState(), s1.getFlags());
	auto state1 = m_scene.getState(true);
	auto state2 = s2.getState();
	m_scene.setAnimationState(state1, state2, s2.getFlags());

	const float deltaFrame = 1.f / m_transitionFrameCount.get();
	const float frameTime = std::clamp(m_transitionFrame.get(), 0, m_transitionFrameCount.get()) * deltaFrame;
	m_scene.setTransitionFrame(frameTime, frameTime + deltaFrame * m_motionBlur.get() * 0.01f, m_transitionExponent.get());
}

SnapshotInterpolator::~SnapshotInterpolator()
{
	m_scene._setState(m_defaultState, m_defaultStateFlags);
	m_scene.loadTransitionState(2);
}

// ------------------------------------------------------------------------ //

CoreInstance & SnapshotInterpolator::core() const
{
	return m_scene.core();
}

void SnapshotInterpolator::pushCommand(QUndoCommand *cmd)
{
	cmd->redo();
	delete cmd;
}

void SnapshotInterpolator::fireChanged(eParamId paramId)
{
	switch (paramId) {
		case PID_SNAPSHOT_INTERPOLATOR_TRANSITION_FRAME:
		case PID_SNAPSHOT_INTERPOLATOR_TRANSITION_FRAME_COUNT:
		case PID_SNAPSHOT_INTERPOLATOR_TRANSITION_EXPONENT:
		case PID_SNAPSHOT_INTERPOLATOR_TRANSITION_MOTION_BLUR: {
			m_transitionFrame.setMax(m_transitionFrameCount.get());
			const float deltaFrame = 1.f / m_transitionFrameCount.get();
			const float frameTime = m_transitionFrame.get() * deltaFrame;
			m_scene.setTransitionFrame(frameTime, frameTime + deltaFrame * m_motionBlur.get() * 0.01f, m_transitionExponent.get());
			break;
		}
	}
}
