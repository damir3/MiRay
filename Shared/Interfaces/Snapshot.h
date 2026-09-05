#pragma once

#include "Parameters.h"
#include <QImage>

enum eSnapshotFlags {
	SF_HAS_CAMERA_STATE			= (1 << 1),
	SF_HAS_TRANSFORMATIONS		= (1 << 2),
	SF_HAS_VISIBILITY			= (1 << 3),
	SF_HAS_ASSIGNED_MATERIALS	= (1 << 4),
	SF_HAS_ENVIRONMENT			= (1 << 5),
	SF_HAS_BACKGROUND			= (1 << 6),
};

struct SnapshotCameraParams {
	vec3	center;
	float	yaw;
	float	pitch;
	float	roll;
	float	distance;
	float	aspect;
	float	fov;
	float	nearZ;
	float	farZ;
	bool	depthOfField;
	float	fStop;
	float	focusDistance;

	SnapshotCameraParams()
		: center(0, 0, 0)
		, yaw(0)
		, pitch(0)
		, roll(0)
		, distance(100)
		, aspect(1)
		, fov(40)
		, nearZ(0.001f)
		, farZ(10000.f)
		, depthOfField(false)
		, fStop(8)
		, focusDistance(100)
	{}
};

struct SnapshotParams {
	QString name;

	bool isCameraValid = false;
	SnapshotCameraParams camera;
};

class ISnapshot
{
protected:
	virtual ~ISnapshot() {}

public:
	virtual IStringParameter & name() = 0;

	virtual SnapshotParams	getParameters() const = 0;

	virtual QImage getPreview() const = 0;

	virtual void activate() const = 0;
	virtual void updateWithCurrent() = 0;

	virtual IBooleanParameter & hasCameraState() = 0;
	virtual IBooleanParameter & hasTransformations() = 0;
	virtual IBooleanParameter & hasVisibility() = 0;
	virtual IBooleanParameter & hasAssignedMaterials() = 0;
	virtual IBooleanParameter & hasEnvironment() = 0;
	virtual IBooleanParameter & hasBackground() = 0;
};
