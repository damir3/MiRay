#pragma once

struct FormatDescriptor {
	enum Flags : uint32_t {
		None               = 0,
		UpY                = 1 << 0,
		FlipFacing         = 1 << 1,
		IgnoreMaterialName = 1 << 2,
		IgnoreOpacity      = 1 << 3,
		TextureOverColor   = 1 << 4,
	};

	const char* id;
	const char* description;
	QStringList extensions;
	uint32_t flags = 0;
	float unitScale = 1.0f;
};

namespace FormatDescriptors {

inline const FormatDescriptor& get3DS()
{
	static const FormatDescriptor desc = {
		"models.loading.3ds",
		"3DS Files",
		{ "3ds" },
		FormatDescriptor::UpY
	};
	return desc;
}

inline const FormatDescriptor& getBlender()
{
	static const FormatDescriptor desc = {
		"models.loading.blend",
		"Blender Files",
		{ "blend" },
		FormatDescriptor::None,
		100.0f
	};
	return desc;
}

inline const FormatDescriptor& getCollada()
{
	static const FormatDescriptor desc = {
		"models.loading.collada",
		"Collada Files",
		{ "dae" },
		FormatDescriptor::UpY | FormatDescriptor::IgnoreOpacity,
		100.0f
	};
	return desc;
}

inline const FormatDescriptor& getDXF()
{
	static const FormatDescriptor desc = {
		"models.loading.dxf",
		"DXF Files",
		{ "dxf" },
		FormatDescriptor::UpY
	};
	return desc;
}

inline const FormatDescriptor& getFBX()
{
	static const FormatDescriptor desc = {
		"models.loading.fbx",
		"FBX Files",
		{ "fbx" },
		FormatDescriptor::UpY
	};
	return desc;
}

inline const FormatDescriptor& getGLTF()
{
	static const FormatDescriptor desc = {
		"models.loading.gltf",
		"glTF/GLB Files",
		{ "gltf", "glb" },
		FormatDescriptor::UpY,
		100.0f
	};
	return desc;
}

inline const FormatDescriptor& getOBJ()
{
	static const FormatDescriptor desc = {
		"models.loading.obj",
		"OBJ Files",
		{ "obj" },
		FormatDescriptor::UpY | FormatDescriptor::FlipFacing | FormatDescriptor::TextureOverColor
	};
	return desc;
}

inline const FormatDescriptor& getPLY()
{
	static const FormatDescriptor desc = {
		"models.loading.ply",
		"PLY Files",
		{ "ply" },
		FormatDescriptor::UpY | FormatDescriptor::FlipFacing | FormatDescriptor::IgnoreMaterialName
	};
	return desc;
}

inline const FormatDescriptor& getSTL()
{
	static const FormatDescriptor desc = {
		"models.loading.stl",
		"STL Files",
		{ "stl" },
		FormatDescriptor::IgnoreMaterialName
	};
	return desc;
}

} // namespace FormatDescriptors
