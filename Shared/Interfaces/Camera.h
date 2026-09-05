#pragma once

#include "Parameters.h"

class IFitToView
{
public:
	virtual ~IFitToView() {}

	virtual IBooleanParameter &justSelectedObjects() = 0;
	virtual IBooleanParameter &includingChildrenObjects() = 0;
	virtual IBooleanParameter &keepAspect() = 0;
	virtual IScalarParameter &padding() = 0;

	virtual void accept() = 0;
};

enum eCameraProjection : uint32_t {
	CP_PERSPECTIVE,
	CP_ORTHOGRAPHIC,
	CP_SPHERICAL,
	CP_CYLINDRICAL,
	CP_FISHEYE,
};

class ICameraProjector
{
public:
	virtual ~ICameraProjector() {}

	virtual eCameraProjection type() const = 0;
	virtual void update(class Camera & camera) = 0;
	virtual bool getRay(vec3 & origin, vec3 & dir, const vec2 & pos, const vec2 * offset = nullptr) const = 0;
	virtual bool getViewportCoords(vec2 & out, const vec3 & pos, bool checkBounds) const = 0;
};

class ICamera
{
protected:
	virtual ~ICamera() {}

public:
	virtual IEnumParameter & projection() = 0;
	virtual IVec3Parameter & target() = 0;
	virtual IScalarParameter & distance() = 0;
	virtual IScalarParameter & yaw() = 0;
	virtual IScalarParameter & pitch() = 0;
	virtual IScalarParameter & roll() = 0;
	virtual IScalarParameter & fov() = 0;
	virtual IScalarParameter & aspect() = 0;
	virtual IScalarParameter & nearZ() = 0;
	virtual IScalarParameter & farZ() = 0;
	virtual IBooleanParameter & depthOfField() = 0;
	virtual IScalarParameter & fStop() = 0;
	virtual IScalarParameter & focusDistance() = 0;
	virtual IIntegerParameter & diaphragmBlades() = 0;
	virtual IScalarParameter & bokehRotation() = 0;
	virtual IScalarParameter & gamma() = 0;

	virtual ICameraProjector * rayProjector() const = 0;

	virtual bool canFitToView() const = 0;
	virtual std::unique_ptr<IFitToView> beginFitToView() = 0;

	virtual void update() = 0;
};
