#pragma once

#include "Parameters.h"
#include "SceneElement.h"

class SHAREDLIB_EXPORT INode : public ISceneElement
{
	Q_OBJECT
	Q_INTERFACES(ISceneElement)

protected:
	virtual ~INode() {}

public:
	virtual IStringParameter & name() = 0;

	virtual IBooleanParameter & visible() = 0;

	virtual IVec3Parameter & position() = 0;
	virtual IVec3Parameter & rotation() = 0;
	virtual IVec3Parameter & scale() = 0;

	// local transformation
	virtual const glm::mat4 & transformation() const = 0;
	virtual void setTransformation(const glm::mat4 & mat) = 0;

	// global transformation
	virtual const glm::mat4 & globalTransformation() const = 0;
	virtual void setGlobalTransformation(const glm::mat4 & mat) = 0;

	virtual const glm::mat4 & inverseGlobalTransformation() const = 0;

	virtual const BBox & oobb() const = 0;
	virtual BBox getAABB(bool checkVisibility = false) const = 0;

	virtual glm::vec3 getObjectBottomPoint(const BBox & bbox) const = 0;
	virtual glm::vec3 getObjectBottomPoint() const = 0;

	virtual INode *addChildNode(const QString & name, eSceneElementType type) = 0;

	virtual void pasteNodes(const QByteArray & data) = 0;
};

Q_DECLARE_INTERFACE(INode, "org.miray.INode")
