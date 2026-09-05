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

#include "../../Shared/Interfaces/ModelLoader.h"
#include "../../Shared/Interfaces/SerializationContext.h"

#include "MiRaySceneSaver.h"

class Scene;
class Camera;
class INode;
class IMeshNode;
class LightNode;
class TextureColorParameterImpl;
class TextureImpl;
class SnapshotImpl;
struct CameraState;

class MiRaySceneLoader
	: public IModelLoader
	, public ISteppedModelLoader
	, public INodeImporter
{
	Q_OBJECT
	Q_INTERFACES(ISteppedModelLoader INodeImporter)

	CoreInstance * m_core;
	IMaterial *    m_defaultMaterial;
	std::vector<const char *> m_chunks;
	const char * m_pos;
	const char * m_chunkEnd;
	const char * m_dataEnd;

	std::map<QString, IMaterial *> m_materialsMap;
	std::vector<int> m_renderLayers;

	std::vector< std::tuple<const char *, const char *, const char *> >              m_materials;
	std::vector< std::tuple<IMeshNode *, const char *, const char *, const char *> > m_geometries;

	QByteArray				m_data;
	ModelLoadingContext	m_context;

	QString correctImagePath(const QString &fileName) const;

	void loadSceneMetadata(Scene & scene);
	void loadProperties(Scene & scene);
	void loadTextureColorParameter(TextureColorParameterImpl & param);
	void loadTexture(TextureImpl & texture);
	void loadCameraState(CameraState & state);
	void loadCamera(Camera & camera);
	void loadSnapshots(Scene & scene);
	void loadSnapshot(Scene & scene);
	void loadMaterials();
	void loadRenderLayers(Scene & scene);
	INode * loadChildNode(INode * node, eChunkType type);
	void loadNode(Node * node);
	void loadMaterial(IMaterialCreator *materialCreator);
	void loadGeometry(IMeshNode * meshNode);
	void loadLight(LightNode * light);
	void loadDirectionalLight(DirectionalLight *light);

	uint32_t readUInt32();
	uint16_t readUInt16();
	float readFloat();
	vec2 readVec2();
	vec3 readVec3();
	vec4 readColor();
	void readData(void *dest, size_t size);
	QString readStr();
	float readIntensity();

	eChunkType openChunk();
	void seekChildrenChunks();
	bool seekNextChunk();
	void closeChunk();

public:

	MiRaySceneLoader();
	~MiRaySceneLoader();

	static const char *uid();

	bool canLoad(const ModelLoadingContext &ctx) const override;

	void preLoad(ModelLoadingContext &ctx, IScene * scene) override;

	int stepCount() const override { return static_cast<int>(m_materials.size() + m_geometries.size()); }

	void loadStep(const ModelLoadingContext &ctx, int i, IMaterialCreator *materialCreator) override;

	void importNodes(INode *dest, IScene *scene, const QByteArray &data) override;

	bool needsAutoSetup() const override { return false; }
};
