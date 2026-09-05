#pragma once

class ISnapshot;
struct SnapshotParams;

class SHAREDLIB_EXPORT ISnapshotManager : public QObject
{
	Q_OBJECT

protected:
	virtual ~ISnapshotManager() {}

public:
	virtual size_t count() const = 0;
	virtual ISnapshot *get(size_t i) const = 0;

	virtual ISnapshot *create() = 0; // stores current camera
	virtual ISnapshot *_create(const SnapshotParams &params) = 0; // parameters-based creation, no undo-redo

	virtual void remove(ISnapshot *p) = 0;
	virtual void move(ISnapshot *p, size_t pos) = 0;

signals:
	void changed();
	void stateChanged(ISnapshot *);
};
