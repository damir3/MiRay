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

#include "SnapshotManager.h"

// ------------------------------------------------------------------------ //

SnapshotManager::SnapshotManager(Scene & scene)
	: m_scene(scene)
{
}

SnapshotManager::~SnapshotManager()
{
}

// ------------------------------------------------------------------------ //

void SnapshotManager::reset()
{
	m_snapshots.clear();
}

// ------------------------------------------------------------------------ //

void SnapshotManager::_add(SnapshotPtr & snapshot, size_t pos)
{
	assert(pos <= m_snapshots.size());
	if (pos < m_snapshots.size())
		m_snapshots.insert(m_snapshots.begin() + pos, std::move(snapshot));
	else
		m_snapshots.push_back(std::move(snapshot));
	fireChanged();
}

SnapshotPtr SnapshotManager::_remove(size_t pos)
{
	assert(pos < m_snapshots.size());
	auto snapshot = std::move(m_snapshots[pos]);
	m_snapshots.erase(m_snapshots.begin() + pos);
	fireChanged();
	return std::move(snapshot);
}

// ------------------------------------------------------------------------ //

class AddSnapshotCommand : public QUndoCommand
{
	SnapshotManager & m_manager;
	SnapshotPtr m_snapshot;
	size_t m_position;

	void redo() override
	{
		m_manager._add(m_snapshot, m_position);
	}

	void undo() override
	{
		m_snapshot = m_manager._remove(m_position);
	}

public:
	AddSnapshotCommand(SnapshotManager & manager, SnapshotPtr snapshot, size_t pos)
		: m_manager(manager)
		, m_snapshot(std::move(snapshot))
		, m_position(pos)
	{
	}
};

class RemoveSnapshotCommand : public QUndoCommand
{
	SnapshotManager & m_manager;
	SnapshotPtr m_snapshot;
	size_t m_position;

	void redo() override
	{
		m_snapshot = m_manager._remove(m_position);
	}

	void undo() override
	{
		m_manager._add(m_snapshot, m_position);
	}

public:
	RemoveSnapshotCommand(SnapshotManager & manager, size_t pos)
		: m_manager(manager)
		, m_position(pos)
	{
	}
};

class MoveSnapshotCommand : public QUndoCommand
{
	SnapshotManager & m_manager;
	size_t m_from, m_to;

	void redo() override
	{
		auto snapshot = m_manager._remove(m_from);
		m_manager._add(snapshot, m_to);
	}

	void undo() override
	{
		auto snapshot = m_manager._remove(m_to);
		m_manager._add(snapshot, m_from);
	}

public:
	MoveSnapshotCommand(SnapshotManager & manager, size_t from, size_t to)
		: m_manager(manager)
		, m_from(from)
		, m_to(to)
	{
	}
};

ISnapshot *SnapshotManager::create()
{
	auto snapshot = new SnapshotImpl(m_scene, QString("Snapshot %1").arg(m_snapshots.size() + 1));
	snapshot->_setState(m_scene.getState(true));
	snapshot->_setPreview(m_scene.getPreview());
	m_scene.pushCommand(new AddSnapshotCommand(*this, SnapshotPtr(snapshot), m_snapshots.size()));
	return snapshot;
}

ISnapshot *SnapshotManager::_create(const QJsonObject &map)
{
	auto name = map["name"].toString();

	auto snapshot = new SnapshotImpl(m_scene, name);
	snapshot->_setState(map);
	snapshot->_setPreview(QImage(1, 1, QImage::Format_RGB32));

	m_snapshots.push_back(SnapshotPtr(snapshot));

	fireChanged();

	return snapshot;
}

ISnapshot *SnapshotManager::_create(const SnapshotParams &params)
{
	QJsonObject map {
		{ "name", params.name }
	};

	if (params.isCameraValid) {
		QJsonObject cam {
			{ "flags", params.camera.depthOfField ? 8 : 0 },
			{ "target", toJsonArray(params.camera.center) },
			{ "rotation", toJsonArray(vec3(params.camera.roll, params.camera.pitch, params.camera.yaw)) },
			{ "dist", params.camera.distance },
			{ "aspect", params.camera.aspect },
			{ "fov", params.camera.fov },
			{ "farZ", params.camera.farZ },
			{ "nearZ", params.camera.nearZ }
		};

		if (params.camera.depthOfField) {
			cam["depthOfField"] = QJsonObject {
				{ "fStop", params.camera.fStop },
				{ "focusDistance", params.camera.focusDistance }
			};
		}

		map["camera"] = cam;
	}

	return _create(map);
}

void SnapshotManager::remove(ISnapshot *p)
{
	auto it = std::find_if(m_snapshots.begin(), m_snapshots.end(), [p](const SnapshotPtr &item) { return item.get() == p; });
	if (it == m_snapshots.end())
		return;

	m_scene.pushCommand(new RemoveSnapshotCommand(*this, std::distance(m_snapshots.begin(), it)));
}

void SnapshotManager::move(ISnapshot *p, size_t pos)
{
	auto it = std::find_if(m_snapshots.begin(), m_snapshots.end(), [p](const SnapshotPtr &item) { return item.get() == p; });
	if (it == m_snapshots.end())
		return;

	auto from = (size_t)std::distance(m_snapshots.begin(), it);
	if (from == pos || pos >= m_snapshots.size())
		return;

	m_scene.pushCommand(new MoveSnapshotCommand(*this, from, pos));
}

// ------------------------------------------------------------------------ //

void SnapshotManager::fireChanged()
{
	emit changed();
}

void SnapshotManager::fireStateChanged(ISnapshot *p)
{
	emit stateChanged(p);
}
