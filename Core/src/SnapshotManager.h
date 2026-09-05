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

class Scene;
#include "SnapshotImpl.h"

class SnapshotManager final : public ISnapshotManager
{
	Q_OBJECT

	Scene &	m_scene;
	SnapshotsList m_snapshots;

	ISnapshot *_create(const QJsonObject &params);

public:
	SnapshotManager(Scene & scene);
	~SnapshotManager();

	void reset();

	size_t count() const override { return m_snapshots.size(); }
	SnapshotImpl *get(size_t i) const override { return m_snapshots[i].get(); }
	SnapshotsList & snapshots() { return m_snapshots; }

	void _add(SnapshotPtr & snapshot, size_t pos);
	SnapshotPtr _remove(size_t pos);

	ISnapshot *create() override;
	ISnapshot *_create(const SnapshotParams &params) override;

	void remove(ISnapshot *p) override;
	void move(ISnapshot *p, size_t pos) override;

	void fireChanged();
	void fireStateChanged(ISnapshot *);
};
