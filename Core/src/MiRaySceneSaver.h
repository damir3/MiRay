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

#include "../../Shared/Interfaces/ModelSaver.h"
#include "../../Shared/Interfaces/SerializationContext.h"

class CoreInstance;
class Scene;
class INode;
class Node;
class LightNode;
class DirectionalLight;
class Camera;
class SnapshotImpl;
class Geometry;
class TextureColorParameterImpl;
struct CameraState;

#define	MIRAY_SCENE_SIGNATURE		"MIRAY SCENE >>>>"
#define	OWLET_SCENE_SIGNATURE		"OWLET SCENE VER1"

enum eChunkType {
	CT_SCENE					= 0xFF000000,
	CT_SCENE_METADATA			= 0xFF000001,

	CT_SCENE_PROPERTIES			= 0xFF000100,
	CT_SCENE_BACKGROUND			= 0xFF000101,
	CT_SCENE_ENVIROMENT			= 0xFF000102,
	CT_SCENE_ENVIROMENT_INTENSITY = 0xFF000103,
	CT_SCENE_DIFFUSE_INTENSITY	= 0xFF000104,
	CT_SCENE_ENVIROMENT_SIZE	= 0xFF000105,
	CT_SCENE_ENVIROMENT_VERTICAL_OFFSET		= 0xFF000106,
	CT_SCENE_ENVIROMENT_HORIZONTAL_ROTATION = 0xFF000107,
	CT_SCENE_ENVIROMENT_VERTICAL_ROTATION	= 0xFF000108,
	CT_SCENE_FLOOR_REFLECTION_LEVEL			= 0xFF000110,
	CT_SCENE_FLOOR_ROUGHNESS				= 0xFF000111,
	CT_SCENE_FLOOR_SHADOW_LEVEL = 0xFF000112,
	CT_SCENE_BACKGROUND_COLOR2	= 0xFF000116,
	CT_SCENE_WATERMARK_FILENAME	= 0xFF000120,
	CT_SCENE_WATERMARK_ALIGNMENT = 0xFF000121,
	CT_SCENE_WATERMARK_SCALE	= 0xFF000122,

	CT_MATERIALS				= 0xFF000200,
	CT_MATERIAL					= 0xFF000201,

	CT_CAMERA					= 0xFF000301,
	CT_CAMERA_POSITION			= 0xFF000302,
	CT_CAMERA_ROTATION			= 0xFF000303,
	CT_CAMERA_DISTANCE			= 0xFF000304,
	CT_CAMERA_FOV				= 0xFF000305,
	CT_CAMERA_ASPECT			= 0xFF000306,
	CT_CAMERA_Z_CLIP			= 0xFF000307,
	CT_CAMERA_FOCUS_DISTANCE	= 0xFF000308,
	CT_CAMERA_F_STOP			= 0xFF000309,
	CT_CAMERA_DIAPHRAGM_BLADES	= 0xFF00030A,
	CT_CAMERA_BOKEH_ROTATION	= 0xFF00030B,
	CT_CAMERA_GAMMA				= 0xFF00030C,

	CT_NODE						= 0xFF000401,
	CT_MESH_NODE				= 0xFF000402,
	CT_LIGHT_NODE				= 0xFF000403,
	CT_CAMERA_NODE				= 0xFF000404,
	CT_DIRECTIONAL_LIGHT_NODE	= 0xFF000405,

	CT_GEOMETRY					= 0xFF000501,
	CT_GEOMETRY_INDICES2		= 0xFF000502,
	CT_GEOMETRY_INDICES4		= 0xFF000503,
	CT_GEOMETRY_POSITIONS		= 0xFF000504,
	CT_GEOMETRY_NORMALS			= 0xFF000505,
	CT_GEOMETRY_UV0				= 0xFF000506,
	CT_GEOMETRY_UV1				= 0xFF000507,
	CT_GEOMETRY_UV2				= 0xFF000508,
	CT_GEOMETRY_UV3				= 0xFF000509,

	CT_SNAPSHOTS				= 0xFF000600,
	CT_SNAPSHOT					= 0xFF000601,
	CT_SNAPSHOT_STATE			= 0xFF000602,
	CT_SNAPSHOT_IMAGE			= 0xFF000603,

	CT_TEXTURE					= 0xFF000701,
	CT_TEXTURE_FILENAME			= 0xFF000702,
	CT_TEXTURE_CROP				= 0xFF000703,
	CT_TEXTURE_REPEAT			= 0xFF000704,
	CT_TEXTURE_OFFSET			= 0xFF000705,
	CT_TEXTURE_ROTATION			= 0xFF000706,
	CT_TEXTURE_WRAP				= 0xFF000707,
	CT_TEXTURE_BRIGHTNESS		= 0xFF000708,
	CT_TEXTURE_CONTRAST			= 0xFF000709,
	CT_TEXTURE_GAMMA			= 0xFF00070A,

	CT_LIGHT					= 0xFF000801,
	CT_LIGHT_COLOR				= 0xFF000802,
	CT_LIGHT_INTENSITY			= 0xFF000803,
	CT_LIGHT_RADIUS				= 0xFF000804,

	CT_DIRECTIONAL_LIGHT				= 0xFF000901,
	CT_DIRECTIONAL_LIGHT_COLOR			= 0xFF000902,
	CT_DIRECTIONAL_LIGHT_INTENSITY		= 0xFF000903,
	CT_DIRECTIONAL_LIGHT_ANGULAR_SIZE	= 0xFF000904,

	CT_RENDER_LAYERS			= 0xFF000A00,
	CT_RENDER_LAYER				= 0xFF000A01
};

class MiRaySceneSaver : public IModelSaver
{
	CoreInstance * m_core;
	std::vector<size_t>	m_chunks;
	size_t	m_stepIndex;
	size_t	m_numSteps;

	QIODevice * m_device;

	ModelSavingContext m_context;

	void writeUInt32(uint32_t n);
	void writeUInt16(uint16_t n);
	void writeFloat(float f);
	void writeData(const void *data, size_t size);
	void writeVec2(const vec2 &v);
	void writeVec3(const vec3 &v);
	void writeColor(const vec4 &c);
	void writeStr(const QString &str);
	void writeData(const QByteArray &data);

	void beginChunk(eChunkType type);
	void updateChunkDataSize();
	void endChunk();

	void saveChunkUInt32(eChunkType type, uint32_t n);
	void saveChunkFloat(eChunkType type, float f);
	void saveChunkVec2(eChunkType type, const vec2 &v);
	void saveChunkVec3(eChunkType type, const vec3 &v);
	void saveChunkColor(eChunkType type, const vec4 &c);
	void saveChunkStr(eChunkType type, const QString &str);
	void saveChunkData(eChunkType type, const QByteArray &data);
	void saveChunkColorParameter(eChunkType type, ColorParameterImpl & param);

	void saveSceneMetadata(Scene &scene);
	void saveProperties(Scene &scene);
	void saveTextureColorParameter(TextureColorParameterImpl & param);
	void saveCameraState(const CameraState &state);
	void saveCamera(const Camera &camera);
	void saveSnapshots(Scene &scene);
	void saveSnapshot(SnapshotImpl &snapshot, IImageManager *imageManager);
	void saveMaterials(IMaterialManager &materialManager);
	void saveMaterial(IMaterial *material);
	void saveRenderLayers(IRenderLayerManager &renderLayerManager);
	void saveRenderLayer(IRenderLayer *renderLayer);
	void saveNode(Node *node, bool globalTransformation = false);
	void saveGeometry(Geometry *geom);
	void saveLight(LightNode *light);
	void saveDirectionalLight(DirectionalLight *light);

	size_t getGeometryCount(ISceneElement *node) const;
	static void findMaterials(Node *node, std::set<IMaterial *> & materials);

public:
	MiRaySceneSaver();
	~MiRaySceneSaver();

	static const char *uid();

	bool save(const ModelSavingContext &ctx, IScene *scene) override;
	QByteArray exportNodes(const NodeSelection &nodes) override;
};
