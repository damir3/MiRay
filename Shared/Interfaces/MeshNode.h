#pragma once

class IGeometry;
class IMaterial;
class IEnumParameter;

class IMeshNode
{
public:
	virtual IGeometry *addGeometry(IMaterial * material) = 0;

	virtual size_t numGeometries() const = 0;
	virtual IGeometry * getGeometry(size_t i) const = 0;
	virtual int getGeometryIndex(const IGeometry * geom) const = 0;

	virtual IEnumParameter & renderLayer() = 0;
};

Q_DECLARE_INTERFACE(IMeshNode, "org.miray.IMeshNode")
