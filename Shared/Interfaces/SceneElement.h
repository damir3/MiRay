#pragma once

enum eSceneElementType {
	SceneElement_Unknown		= 0, // use for deprecated elements or for defaults
	SceneElement_Node			= 1 << 0,
	SceneElement_MeshNode		= 1 << 1,
	SceneElement_Light			= 1 << 2,
	SceneElement_DirectionalLight	= 1 << 3,
	SceneElement_AnyNode		= SceneElement_Node | SceneElement_MeshNode | SceneElement_Light | SceneElement_DirectionalLight,
	SceneElement_Geometry		= 1 << 6,
	SceneElement_All			= 0xFFFF,
};

class INode;

class SHAREDLIB_EXPORT ISceneElement : public QObject
{
	Q_OBJECT

protected:
	virtual ~ISceneElement() {}

public:
	virtual eSceneElementType type() const = 0;
	virtual QString name() const = 0;

	virtual size_t numChildren(eSceneElementType) const = 0;
	virtual ISceneElement * child(size_t pos, eSceneElementType) const = 0;
	virtual INode * parent() const = 0;
};

Q_DECLARE_INTERFACE(ISceneElement, "org.miray.ISceneElement")
