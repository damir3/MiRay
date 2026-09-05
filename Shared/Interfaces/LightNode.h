#pragma once

class IColorParameter;

class ILightNode
{
public:
	virtual IColorParameter & color() = 0;
	virtual IScalarParameter & intensity() = 0;
	virtual IScalarParameter & radius() = 0;
};

Q_DECLARE_INTERFACE(ILightNode, "org.miray.ILightNode")

class IDirectionalLight
{
public:
	virtual IColorParameter & color() = 0;
	virtual IScalarParameter & intensity() = 0;
	virtual IScalarParameter & angularSize() = 0;

	virtual IScalarParameter & yaw() = 0;
	virtual IScalarParameter & pitch() = 0;
};

Q_DECLARE_INTERFACE(IDirectionalLight, "org.miray.IDirectionalLight")
