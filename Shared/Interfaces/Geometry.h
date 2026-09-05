#pragma once

#include "SceneElement.h"

class IMaterial;

enum { MAX_UV_SETS = 4 };

class SHAREDLIB_EXPORT IGeometry : public ISceneElement
{
	Q_OBJECT
	Q_INTERFACES(ISceneElement)

protected:
	virtual ~IGeometry() {}

public:
	virtual IMaterial * material() const = 0;
	virtual size_t numVertices() const = 0;
	virtual size_t numIndices() const = 0;

	virtual const std::vector<Vertex> &vertices() const = 0;
	virtual const std::vector<uint32_t> &indices() const = 0;
	virtual const std::vector<glm::vec2> &uvSet(int i) const = 0;

	virtual void setVertices(const std::vector<Vertex> & verts) = 0;
	virtual void setIndices(const std::vector<uint32_t> & indices) = 0;
	virtual void setUVset(int i, const std::vector<glm::vec2> & uvSet) = 0;
};

Q_DECLARE_INTERFACE(IGeometry, "org.miray.IGeometry")
