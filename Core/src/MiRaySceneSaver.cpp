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

#include "MiRaySceneSaver.h"

// ------------------------------------------------------------------------ //

MiRaySceneSaver::MiRaySceneSaver()
	: m_core(nullptr)
	, m_device(nullptr)
{
}

MiRaySceneSaver::~MiRaySceneSaver()
{
}

const char *MiRaySceneSaver::uid()
{
	return "models.saving.mirayScene";
}

// ------------------------------------------------------------------------ //

bool MiRaySceneSaver::save(const ModelSavingContext &ctx, IScene *sceneParam)
{
	m_context = ctx;

	QFile file(ctx.targetFileName);
	if (!file.open(QIODevice::WriteOnly))
		throw std::runtime_error("Can't open the file for writing");

	m_device = &file;

	m_chunks.clear();

	auto & scene = *static_cast<Scene *>(sceneParam);
	m_core = &scene.core();
	m_stepIndex = 0;
	m_numSteps = scene.materialManager().count() + getGeometryCount(&scene.root());

	writeData(MIRAY_SCENE_SIGNATURE, 16);
	beginChunk(CT_SCENE);
	writeUInt16((VER_MAJOR << 8) | VER_MINOR); // major and minor versions
	writeUInt16(VER_BUILD); // build version
	updateChunkDataSize();

	saveSceneMetadata(scene);
	saveProperties(scene);
	saveCamera(scene.camera());
	saveSnapshots(scene);

	saveMaterials(scene.materialManager());

	saveRenderLayers(scene.renderLayerManager());

	saveNode(static_cast<Node *>(&scene.root()));

	endChunk();

	m_device = nullptr;
	file.close();

	return true;
}

void MiRaySceneSaver::findMaterials(Node *node, std::set<IMaterial *> & materials)
{
	if (node->isMeshNode()) {
		if (auto meshNode = qobject_cast<MeshNode *>(node)) {
			for (const auto & geom : meshNode->geometries()) {
				auto material = geom->material();
				if (!material->guid().isNull())
					materials.insert(material);
			}
		}
	}

	auto nodesCount = node->numChildren(SceneElement_Node);
	for (size_t i = 0; i < nodesCount; i++)
		findMaterials(static_cast<Node *>(node->child(i, SceneElement_Node)), materials);
}

QByteArray MiRaySceneSaver::exportNodes(const NodeSelection &nodes)
{
	QBuffer buffer;
	buffer.open(QBuffer::WriteOnly);
	m_device = &buffer;

	{// save materials
		std::set<IMaterial *> materials;
		for (auto node : nodes)
			findMaterials(static_cast<Node *>(node), materials);

		if (!materials.empty()) {
			beginChunk(CT_MATERIALS);

			for (auto material : materials)
				saveMaterial(material);

			endChunk();
		}
	}

	for (auto node : nodes)
		saveNode(static_cast<Node *>(node), true);

	buffer.close();
	return buffer.data();
}

// ------------------------------------------------------------------------ //

size_t MiRaySceneSaver::getGeometryCount(ISceneElement *node) const
{
	if (node->type() == SceneElement_Geometry)
		return 1;

	size_t count = 0;
	for (size_t i = 0; i < node->numChildren(SceneElement_All); i++)
		count += getGeometryCount(node->child(i, SceneElement_All));

	return count;
}

// ------------------------------------------------------------------------ //

void MiRaySceneSaver::saveSceneMetadata(Scene &scene)
{
	beginChunk(CT_SCENE_METADATA);
	QByteArray data;
	QDataStream stream(&data, QIODevice::WriteOnly);
	stream.setVersion(QDataStream::Qt_4_8);
	stream << scene.metadata();
	writeUInt32(data.size());
	writeData(data);
	updateChunkDataSize();
	endChunk();
}

void MiRaySceneSaver::saveProperties(Scene &scene)
{
	beginChunk(CT_SCENE_PROPERTIES);
	writeUInt32((uint32_t)scene.backgroundMode().getIndex());
	updateChunkDataSize();

	saveChunkColorParameter(CT_SCENE_BACKGROUND, scene.background());
	saveChunkColorParameter(CT_SCENE_BACKGROUND_COLOR2, scene.backgroundColor2());
	saveChunkColorParameter(CT_SCENE_ENVIROMENT, scene.environment());

	saveChunkFloat(CT_SCENE_ENVIROMENT_INTENSITY, scene.environmentIntensity().get() * 0.01f);
	saveChunkFloat(CT_SCENE_DIFFUSE_INTENSITY, scene.diffuseIntensity().get() * 0.01f);
	saveChunkFloat(CT_SCENE_ENVIROMENT_SIZE, scene.environmentSize().get());
	saveChunkFloat(CT_SCENE_ENVIROMENT_VERTICAL_OFFSET, scene.environmentVerticalOffset().get());
	saveChunkFloat(CT_SCENE_ENVIROMENT_HORIZONTAL_ROTATION, scene.environmentHorizontalRotation().get());
	saveChunkFloat(CT_SCENE_ENVIROMENT_VERTICAL_ROTATION, scene.environmentVerticalRotation().get());

	saveChunkFloat(CT_SCENE_FLOOR_REFLECTION_LEVEL, scene.floorReflectionLevel().get() * 0.01f);
	saveChunkFloat(CT_SCENE_FLOOR_ROUGHNESS, scene.floorRoughness().get() * 0.01f);
	saveChunkFloat(CT_SCENE_FLOOR_SHADOW_LEVEL, scene.floorShadowLevel().get() * 0.01f);

	endChunk();
}

void MiRaySceneSaver::saveChunkColorParameter(eChunkType type, ColorParameterImpl & param)
{
	beginChunk(type);
	writeColor(vec4(param.get(), 1.f));
	updateChunkDataSize();

	auto texture = static_cast<TextureImpl *>(param.texture());
	if (texture) {
		beginChunk(CT_TEXTURE);

		uint16_t flags = 0;
		if (texture->enabled().get())	flags |= 1;
		if (texture->invert().get())	flags |= 2;
		writeUInt16(flags);
		updateChunkDataSize();

		auto fname = texture->fileName().get();
		if (!m_context.mapFileNamesOverride.empty() && m_context.mapFileNamesOverride.contains(fname)) {
			// resources collection
			fname = m_context.mapFileNamesOverride[fname];
		} else {
			// standard saving
			if (qobject_cast<IApplicationContext *>(qApp)->isInternalResource(fname))
				fname = qobject_cast<IApplicationContext *>(qApp)->getShortResourcePath(fname);
			else
				fname = relativePath(fname, m_context.targetFolder);
		}

		saveChunkStr(CT_TEXTURE_FILENAME, fname);
		saveChunkVec2(CT_TEXTURE_REPEAT, texture->repeat().get());
		saveChunkVec2(CT_TEXTURE_OFFSET, texture->offset().get());
		saveChunkFloat(CT_TEXTURE_ROTATION, texture->rotation().get());

		beginChunk(CT_TEXTURE_CROP);
		writeFloat(texture->cropLeft().get());
		writeFloat(texture->cropTop().get());
		writeFloat(texture->cropRight().get());
		writeFloat(texture->cropBottom().get());
		updateChunkDataSize();
		endChunk();

		beginChunk(CT_TEXTURE_WRAP);
		writeUInt16(texture->wrapX().getIndex());
		writeUInt16(texture->wrapY().getIndex());
		updateChunkDataSize();
		endChunk();

		saveChunkFloat(CT_TEXTURE_BRIGHTNESS, texture->brightness().get());
		saveChunkFloat(CT_TEXTURE_CONTRAST, texture->contrast().get());
		saveChunkFloat(CT_TEXTURE_GAMMA, texture->gamma().get());

		endChunk(); // CT_TEXTURE
	}

	endChunk();
}

void MiRaySceneSaver::saveCameraState(const CameraState &state)
{
	uint32_t flags = state.projection;
	if (state.depthOfField) flags |= (1 << 8);
	writeUInt32(flags); // flags
	updateChunkDataSize();

	saveChunkVec3(CT_CAMERA_POSITION, state.target);
	saveChunkVec3(CT_CAMERA_ROTATION, vec3(state.yaw, state.pitch, state.roll));
	saveChunkFloat(CT_CAMERA_DISTANCE, state.distance);
	saveChunkFloat(CT_CAMERA_FOV, state.fov);
	saveChunkFloat(CT_CAMERA_ASPECT, state.aspect);
	saveChunkVec2(CT_CAMERA_Z_CLIP, vec2(state.nearZ, state.farZ));
	saveChunkFloat(CT_CAMERA_FOCUS_DISTANCE, state.focusDistance);
	saveChunkFloat(CT_CAMERA_F_STOP, state.fStop);
	saveChunkUInt32(CT_CAMERA_DIAPHRAGM_BLADES, state.diaphragmBlades);
	saveChunkFloat(CT_CAMERA_BOKEH_ROTATION, state.bokehRotation);
	saveChunkFloat(CT_CAMERA_GAMMA, state.gamma);
}

void MiRaySceneSaver::saveCamera(const Camera &camera)
{
	beginChunk(CT_CAMERA);
	saveCameraState(camera.getCameraState());
	endChunk();
}

void MiRaySceneSaver::saveSnapshots(Scene &scene)
{
	auto & snapshotManager = scene.snapshotManager();

	beginChunk(CT_SNAPSHOTS);
	writeUInt32((uint32_t)snapshotManager.count());
	updateChunkDataSize();

	for (size_t i = 0; i < snapshotManager.count(); i++)
		saveSnapshot(*snapshotManager.get(i), scene.context().imageManager());

	endChunk();
}

void MiRaySceneSaver::saveSnapshot(SnapshotImpl &snapshot, IImageManager *imageManager)
{
	beginChunk(CT_SNAPSHOT);
	writeUInt32(snapshot.getFlags());
	writeStr(snapshot.name().get());
	writeStr(snapshot.guid());
	updateChunkDataSize();

	beginChunk(CT_SNAPSHOT_STATE);
	auto data = QJsonDocument(snapshot.getState()).toJson(QJsonDocument::Compact);
	data = qCompress(data);
	writeData(data);
	updateChunkDataSize();
	endChunk();

	endChunk();
}

void MiRaySceneSaver::saveMaterials(IMaterialManager & materialManager)
{
	beginChunk(CT_MATERIALS);

	for (int i = 0; i < materialManager.count(); i++) {
		saveMaterial(materialManager.get(i));

		emit updateProgress((float)(++m_stepIndex) / m_numSteps);
	}

	endChunk();
}

void MiRaySceneSaver::saveMaterial(IMaterial *material)
{
	beginChunk(CT_MATERIAL);
	auto data = material->save(m_context);
	writeData(data);
	updateChunkDataSize();
	endChunk();
}

void MiRaySceneSaver::saveRenderLayers(IRenderLayerManager &renderLayerManager)
{
	beginChunk(CT_RENDER_LAYERS);

	for (int i = 0; i < renderLayerManager.count(); i++)
		saveRenderLayer(renderLayerManager.get(i));

	endChunk();
}

void MiRaySceneSaver::saveRenderLayer(IRenderLayer *renderLayer)
{
	beginChunk(CT_RENDER_LAYER);
	writeStr(renderLayer->name().get());
	updateChunkDataSize();
	endChunk();
}

void MiRaySceneSaver::saveNode(Node *node, bool globalTransformation)
{
	eChunkType ct;
	switch (node->type()) {
		case SceneElement_Node:				ct = CT_NODE; break;
		case SceneElement_MeshNode:			ct = CT_MESH_NODE; break;
		case SceneElement_Light:			ct = CT_LIGHT_NODE; break;
		case SceneElement_DirectionalLight:	ct = CT_DIRECTIONAL_LIGHT_NODE; break;
		default: return;
	}

	auto * meshNode = node->isMeshNode() ? qobject_cast<MeshNode *>(node) : nullptr;

	uint32_t flags = node->visible().get() ? 1 : 0;
	flags |= meshNode ? (meshNode->renderLayer().getIndex() << 16) : 0;

	beginChunk(ct);
	writeUInt32(flags);
	writeData(glm::value_ptr(globalTransformation ? node->globalTransformation() : node->transformation()), sizeof(mat4));
	writeStr(node->name().get());
	writeStr(node->guid());
	updateChunkDataSize();

	switch (node->type()) {
		case SceneElement_MeshNode:
			if (meshNode) {
				for (const auto & geom : meshNode->geometries())
					saveGeometry(geom.get());
			}
			break;

		case SceneElement_Light:
			if (auto lightNode = qobject_cast<LightNode *>(node))
				saveLight(lightNode);
			break;

		case SceneElement_DirectionalLight:
			if (auto lightNode = qobject_cast<DirectionalLight *>(node))
				saveDirectionalLight(lightNode);
			break;
	}

	auto nodesCount = node->numChildren(SceneElement_Node);
	for (size_t i = 0; i < nodesCount; i++)
		saveNode(static_cast<Node *>(node->child(i, SceneElement_Node)));

	endChunk();
}

QByteArray CompressData(const char *src, size_t num, size_t elemSize, size_t pitch)
{
	QByteArray out;
	out.reserve(static_cast<int>(num * elemSize));
	for (size_t i = 0; i < num; i++) {
		out.append(src, static_cast<int>(elemSize));
		src += pitch;
	}
	return qCompress(out);
}

void MiRaySceneSaver::saveGeometry(Geometry *geom)
{
	auto vertices = geom->vertices();
	auto indices = geom->indices();

	beginChunk(CT_GEOMETRY);
	writeUInt32(0 | 1 | 2); // flags (position + normal + texcoord0)
	writeUInt32((uint32_t)vertices.size());
	writeUInt32((uint32_t)indices.size());
	writeStr(geom->material()->name().get());
	writeStr(geom->guid());
	updateChunkDataSize();

	saveChunkData(CT_GEOMETRY_POSITIONS, CompressData((char *)&vertices.data()->pos, vertices.size(), 3 * sizeof(float), sizeof(Vertex)));
	saveChunkData(CT_GEOMETRY_NORMALS, CompressData((char *)&vertices.data()->normal, vertices.size(), 3 * sizeof(float), sizeof(Vertex)));

	if (vertices.size() < 0x10000) {
		beginChunk(CT_GEOMETRY_INDICES2);
		writeData(CompressData((char *)indices.data(), indices.size(), 2, 4));
	} else {
		beginChunk(CT_GEOMETRY_INDICES4);
		QByteArray data = QByteArray::fromRawData((char *)indices.data(), static_cast<int>(indices.size() * 4));
		data = qCompress(data);
		writeData(data);
	}
	updateChunkDataSize();
	endChunk();

	for (int c = 0; c < MAX_UV_SETS; c++) {
		auto & uvSet = geom->uvSet(c);
		if (uvSet.empty()) continue;
		saveChunkData((eChunkType)(CT_GEOMETRY_UV0 + c), CompressData((char *)uvSet.data(), uvSet.size(), 2 * sizeof(float), sizeof(vec2)));
	}

	endChunk();

	emit updateProgress((float)(++m_stepIndex) / m_numSteps);
}

void MiRaySceneSaver::saveLight(LightNode *light)
{
	beginChunk(CT_LIGHT);
	writeUInt32(0); // flags
	updateChunkDataSize();

	saveChunkVec3(CT_LIGHT_COLOR, light->color().sRGB());
	saveChunkFloat(CT_LIGHT_INTENSITY, light->intensity().get());
	saveChunkFloat(CT_LIGHT_RADIUS, light->radius().get());

	endChunk();
}

void MiRaySceneSaver::saveDirectionalLight(DirectionalLight *light)
{
	beginChunk(CT_DIRECTIONAL_LIGHT);
	writeUInt32(0); // flags
	updateChunkDataSize();

	saveChunkVec3(CT_DIRECTIONAL_LIGHT_COLOR, light->color().sRGB());
	saveChunkFloat(CT_DIRECTIONAL_LIGHT_INTENSITY, light->intensity().get() * 0.01f);
	saveChunkFloat(CT_DIRECTIONAL_LIGHT_ANGULAR_SIZE, light->angularSize().get());

	endChunk();
}

// ------------------------------------------------------------------------ //

void MiRaySceneSaver::beginChunk(eChunkType type)
{
	m_chunks.push_back(static_cast<size_t>(m_device->pos()));
	writeUInt32(type);
	writeUInt32(0); // chunk size
	writeUInt32(0); // data size
}

void MiRaySceneSaver::updateChunkDataSize()
{
	auto begin = m_chunks.back();
	auto end = static_cast<size_t>(m_device->pos());
	m_device->seek(begin + 8);
	writeUInt32((uint32_t)((size_t)end - begin - 12));
	m_device->seek(end);
}

void MiRaySceneSaver::endChunk()
{
	auto begin = m_chunks.back();
	auto end = static_cast<size_t>(m_device->pos());
	m_device->seek(begin + 4);
	writeUInt32((uint32_t)((size_t)end - begin));
	m_device->seek(end);
	m_chunks.pop_back();
}

// ------------------------------------------------------------------------ //

void MiRaySceneSaver::writeUInt32(uint32_t n)
{
	m_device->write((char *)&n, 4);
}

void MiRaySceneSaver::writeUInt16(uint16_t n)
{
	m_device->write((char *)&n, 2);
}

void MiRaySceneSaver::writeFloat(float f)
{
	m_device->write((char *)&f, 4);
}

void MiRaySceneSaver::writeData(const void * data, size_t size)
{
	m_device->write((char *)data, size);
}

void MiRaySceneSaver::writeVec2(const vec2 & v)
{
	m_device->write((char *)&v, sizeof(v));
}

void MiRaySceneSaver::writeVec3(const vec3 & v)
{
	m_device->write((char *)&v, sizeof(v));
}

void MiRaySceneSaver::writeColor(const vec4 & c)
{
	m_device->write((char *)&c, sizeof(c));
}

void MiRaySceneSaver::writeData(const QByteArray & data)
{
	writeData(data.constData(), data.size());
}

void MiRaySceneSaver::writeStr(const QString & str)
{
	auto data = str.toUtf8();
	writeUInt16(data.size());
	writeData(data.constData(), data.size());
}

void MiRaySceneSaver::saveChunkUInt32(eChunkType type, uint32_t n)
{
	beginChunk(type);
	writeUInt32(n);
	updateChunkDataSize();
	endChunk();
}

void MiRaySceneSaver::saveChunkFloat(eChunkType type, float f)
{
	beginChunk(type);
	writeFloat(f);
	updateChunkDataSize();
	endChunk();
}

void MiRaySceneSaver::saveChunkVec2(eChunkType type, const vec2 & v)
{
	beginChunk(type);
	writeVec2(v);
	updateChunkDataSize();
	endChunk();
}

void MiRaySceneSaver::saveChunkVec3(eChunkType type, const vec3 & v)
{
	beginChunk(type);
	writeVec3(v);
	updateChunkDataSize();
	endChunk();
}

void MiRaySceneSaver::saveChunkColor(eChunkType type, const vec4 & c)
{
	beginChunk(type);
	writeColor(c);
	updateChunkDataSize();
	endChunk();
}

void MiRaySceneSaver::saveChunkStr(eChunkType type, const QString & str)
{
	beginChunk(type);
	writeStr(str);
	updateChunkDataSize();
	endChunk();
}

void MiRaySceneSaver::saveChunkData(eChunkType type, const QByteArray & data)
{
	beginChunk(type);
	writeData(data);
	updateChunkDataSize();
	endChunk();
}
