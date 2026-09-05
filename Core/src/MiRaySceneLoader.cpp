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

#include "MiRaySceneLoader.h"

#include "../../Shared/Utils/FileUtils.h"

#define MATERIAL_NAME				"name"

// ------------------------------------------------------------------------ //

const char *MiRaySceneLoader::uid()
{
	return "models.loading.mirayScene";
}

// ------------------------------------------------------------------------ //

MiRaySceneLoader::MiRaySceneLoader()
	: m_core(nullptr)
{
}

MiRaySceneLoader::~MiRaySceneLoader()
{
}

// ------------------------------------------------------------------------ //

uint32_t MiRaySceneLoader::readUInt32()
{
	if (m_pos + 4 > m_dataEnd)
		throw std::runtime_error("Read uint32 error: unexpected end of chunk data.");

	auto n = *(uint32_t *)m_pos;
	m_pos += 4;
	return n;
}

uint16_t MiRaySceneLoader::readUInt16()
{
	if (m_pos + 2 > m_dataEnd)
		throw std::runtime_error("Read uint16 error: unexpected end of chunk data.");

	auto n = *(uint32_t *)m_pos;
	m_pos += 2;
	return n;
}

float MiRaySceneLoader::readFloat()
{
	if (m_pos + 4 > m_dataEnd)
		throw std::runtime_error("Read float error: unexpected end of chunk data.");

	auto f = *(float *)m_pos;
	m_pos += 4;
	return f;
}

vec2 MiRaySceneLoader::readVec2()
{
	if (m_pos + 8 > m_dataEnd)
		throw std::runtime_error("Read vec2 error: unexpected end of chunk data.");

	vec2 v = *(vec2 *)m_pos;
	m_pos += 8;
	return v;
}

vec3 MiRaySceneLoader::readVec3()
{
	if (m_pos + 12 > m_dataEnd)
		throw std::runtime_error("Read vec3 error: unexpected end of chunk data.");

	vec3 v = *(vec3 *)m_pos;
	m_pos += 12;
	return v;
}

vec4 MiRaySceneLoader::readColor()
{
	if (m_pos + 16 > m_dataEnd)
		throw std::runtime_error("Read color error: unexpected end of chunk data.");

	auto n = *(vec4 *)m_pos;
	m_pos += 16;
	return n;
}

void MiRaySceneLoader::readData(void *dest, size_t size)
{
	if (m_pos + size > m_dataEnd)
		throw std::runtime_error("Read data error: unexpected end of chunk data.");

	memcpy(dest, m_pos, size);
	m_pos += size;
}

QString MiRaySceneLoader::readStr()
{
	auto size = readUInt16();
	if (m_pos + size > m_dataEnd)
		throw std::runtime_error("Read string error: unexpected end of chunk data.");

	QString out = QString::fromUtf8(m_pos, size);
	m_pos += size;
	return out;
}

float MiRaySceneLoader::readIntensity()
{
	const auto intensity = readFloat();
	if (m_context.fileVersion < 0x0106)
		return ColorUtils::sRGBToLinear(intensity);

	return intensity;
}

// ------------------------------------------------------------------------ //

eChunkType MiRaySceneLoader::openChunk()
{
	if (m_pos + 12 > m_chunkEnd)
		throw std::runtime_error("OpenChunk failed: unexpected end of data.");

	struct ChunkHeader {
		uint32_t	type;
		uint32_t	chunkSize;
		uint32_t	dataSize;
	};
	ChunkHeader *chunk = (ChunkHeader *)m_pos;
	m_chunkEnd = m_pos + chunk->chunkSize;
	if (m_chunkEnd > m_chunks.back())
		throw std::runtime_error("OpenChunk failed: invalid chunk size.");

	m_pos += 12;
	m_dataEnd = m_pos + chunk->dataSize;
	if (m_dataEnd > m_chunkEnd)
		throw std::runtime_error("OpenChunk failed: invalid chunk data size.");

	m_chunks.push_back(m_chunkEnd);

	return (eChunkType)chunk->type;
}

void MiRaySceneLoader::seekChildrenChunks()
{
	m_pos = m_dataEnd;
}

void MiRaySceneLoader::closeChunk()
{
	m_pos = m_chunkEnd;
	m_chunks.pop_back();
	m_chunkEnd = m_chunks.back();
}

// ------------------------------------------------------------------------ //

bool MiRaySceneLoader::canLoad(const ModelLoadingContext &ctx) const
{
	auto suffix = QFileInfo(ctx.sourceFileName).suffix();
	if (suffix.compare("mirayScene", Qt::CaseInsensitive) == 0 ||
		suffix.compare("owletScene", Qt::CaseInsensitive) == 0)
		return true;
	return false;
}

void MiRaySceneLoader::preLoad(ModelLoadingContext &ctx, IScene * sceneParam)
{
	LogInformation() << QString("MiRay scene loader is about to load scene '%1'").arg(ctx.sourceFileName);

	m_data = readFile(ctx.sourceFileName);

	if (memcmp(m_data.data(), MIRAY_SCENE_SIGNATURE, 16) != 0 &&
		memcmp(m_data.data(), OWLET_SCENE_SIGNATURE, 16) != 0)
		throw std::runtime_error("Invalid file signature.");

	auto & scene = *static_cast<Scene *>(sceneParam);
	m_core = &scene.core();
	m_defaultMaterial = scene.materialManager().getDefault();
	m_pos = m_data.data() + 16;
	m_chunkEnd = m_pos + m_data.size();
	m_chunks.push_back(m_chunkEnd);

	auto type = openChunk();
	if (type != CT_SCENE)
		throw std::runtime_error("Invalid file header.");

	ctx.fileVersion = readUInt16(); // major and minor versions
	auto buildVersion = readUInt16(); // build version

	m_context = ctx;

	LogInformation() << QString().asprintf("File format version: %d.%d.%d", m_context.fileVersion >> 8, m_context.fileVersion & 0xFF, buildVersion);

	uint16_t cur_version = (VER_MAJOR << 8) + VER_MINOR;
	if (cur_version < m_context.fileVersion) {
		int file_minor = m_context.fileVersion & 0xFF;
		int file_major = m_context.fileVersion >> 8;
		throw std::runtime_error(QString("This project needs at least MiRay %1.%2, while you have version %3\nPlease upgrade your copy of MiRay to load this project.").arg(file_major).arg(file_minor).arg(qApp->applicationVersion()).toStdString());
	}

	seekChildrenChunks();

	while (m_pos < m_chunkEnd) {
		type = openChunk();
		switch (type) {
			case CT_SCENE_METADATA:
				if (ctx.loadNewScene)
					loadSceneMetadata(scene);
				break;

			case CT_SCENE_PROPERTIES:
				if (ctx.loadNewScene)
					loadProperties(scene);
				break;

			case CT_CAMERA:
				if (ctx.loadNewScene)
					loadCamera(scene.camera());
				break;

			case CT_SNAPSHOTS:
				loadSnapshots(scene);
				break;

			case CT_MATERIALS:
				loadMaterials();
				break;

			case CT_RENDER_LAYERS:
				loadRenderLayers(scene);
				break;

			case CT_NODE: {
				auto root = static_cast<Node *>(&scene.root());
				auto flags = readUInt32();
				if (ctx.loadNewScene) {
					mat4 mat;
					readData(&mat, sizeof(mat4));
					root->setTransformation(mat);
					root->name()._set(readStr());
					root->visible()._set(flags & 1);
					if (m_context.fileVersion >= 0x0103 && m_context.loadNewScene)
						root->_setGuid(readStr());
				}
				loadNode(root);
				break;
			}
		}
		closeChunk();
	}

	closeChunk();
}

void MiRaySceneLoader::loadSceneMetadata(Scene & scene)
{
	auto size = (size_t)readUInt32();
	auto maxSize = (size_t)std::distance(m_pos, m_dataEnd);
	if (size > maxSize)
		throw std::runtime_error("Metadata reading error.");

	QByteArray data;
	data.resize(static_cast<int>(size));
	readData(data.data(), size);
	QDataStream stream(data);
	stream.setVersion(QDataStream::Qt_4_8);
	QJsonObject metadata;
	stream >> metadata;
	scene.setMetadata(metadata);
}

void MiRaySceneLoader::loadProperties(Scene &scene)
{
	uint32_t flags = readUInt32();
	scene.backgroundMode()._setIndex(flags & (m_context.fileVersion < 0x0200 ? 3 : 7));

	seekChildrenChunks();

	scene.environmentSize()._set(0.f);
	scene.diffuseIntensity()._set(100.f);

	while (m_pos < m_chunkEnd) {
		auto type = openChunk();
		switch (type) {
			case CT_SCENE_BACKGROUND:
				loadTextureColorParameter(scene.background());
				break;

			case CT_SCENE_BACKGROUND_COLOR2:
				scene.backgroundColor2()._set(readColor());
				break;

			case CT_SCENE_ENVIROMENT:
				loadTextureColorParameter(scene.environment());
				break;

			case CT_SCENE_ENVIROMENT_INTENSITY:
				scene.environmentIntensity()._set(readIntensity() * 100.f); // linear
				break;

			case CT_SCENE_DIFFUSE_INTENSITY:
				scene.diffuseIntensity()._set(readFloat() * 100.f); // srgb
				break;

			case CT_SCENE_ENVIROMENT_SIZE:
				scene.environmentSize()._set(readFloat());
				break;

			case CT_SCENE_ENVIROMENT_VERTICAL_OFFSET:
				scene.environmentVerticalOffset()._set(readFloat());
				break;

			case CT_SCENE_ENVIROMENT_HORIZONTAL_ROTATION:
				scene.environmentHorizontalRotation()._set(readFloat());
				break;

			case CT_SCENE_ENVIROMENT_VERTICAL_ROTATION:
				scene.environmentVerticalRotation()._set(readFloat());
				break;

			case CT_SCENE_FLOOR_REFLECTION_LEVEL:
				scene.floorReflectionLevel()._set(readFloat() * 100.f);
				break;

			case CT_SCENE_FLOOR_ROUGHNESS: {
				const auto roughness = readFloat();
				scene.floorRoughness()._set((m_context.fileVersion < 0x0107 ? std::sqrt(roughness) : roughness) * 100.f);
				break;
			}

			case CT_SCENE_FLOOR_SHADOW_LEVEL:
				scene.floorShadowLevel()._set(readFloat() * 100.f);
				break;
		}
		closeChunk();
	}
}

QString MiRaySceneLoader::correctImagePath(const QString &fileName) const
{
	if (qobject_cast<IApplicationContext *>(qApp)->isInternalResource(fileName))
		return qobject_cast<IApplicationContext *>(qApp)->getShortResourcePath(fileName);

	return resolveFilePath(fileName, m_context.sourceFolder);
}

void MiRaySceneLoader::loadTextureColorParameter(TextureColorParameterImpl & param)
{
	auto color = readColor();
	seekChildrenChunks();

	while (m_pos < m_chunkEnd) {
		auto type = openChunk();
		switch (type) {
			case CT_TEXTURE:
				loadTexture(*param.texture());
				param.fireChanged(PID_TEXTURE);
				break;
		}
		closeChunk();
	}

	param._set(color);
}

void MiRaySceneLoader::loadTexture(TextureImpl & texture)
{
	auto flags = readUInt16();
	seekChildrenChunks();

	QString fileName;
	RectF crop(0.f, 0.f, 1.f, 1.f);
	int mapping = texture.mapping().getIndex();
	uint16_t wrap[2] = {TextureImpl::WRAP_REPEAT, TextureImpl::WRAP_REPEAT};
	vec2 repeat(1.f);
	vec2 offset(0.f);
	float rotation = 0.f;
	float brightness = 0.f;
	float contrast = 0.f;
	float gamma = 1.f;

	while (m_pos < m_chunkEnd) {
		auto type = openChunk();
		switch (type) {
			case CT_TEXTURE_FILENAME:
				fileName = correctImagePath(readStr());
				break;

			case CT_TEXTURE_REPEAT:
				repeat = readVec2();
				break;

			case CT_TEXTURE_OFFSET:
				offset = readVec2();
				break;

			case CT_TEXTURE_ROTATION:
				rotation = readFloat();
				break;

			case CT_TEXTURE_CROP:
				crop.left = readFloat();
				crop.top = readFloat();
				crop.right = readFloat();
				crop.bottom = readFloat();
				break;

			case CT_TEXTURE_WRAP:
				wrap[0] = readUInt16();
				wrap[1] = readUInt16();
				break;

			case CT_TEXTURE_BRIGHTNESS:
				brightness = readFloat();
				break;

			case CT_TEXTURE_CONTRAST:
				contrast = readFloat();
				break;

			case CT_TEXTURE_GAMMA:
				gamma = readFloat();
				break;
		}
		closeChunk();
	}

	texture._set(flags & 1, fileName, crop, (TextureImpl::eWrap)wrap[0], (TextureImpl::eWrap)wrap[1],
						(TextureImpl::eMapping)mapping, repeat, offset, rotation, flags & 2,
						brightness, contrast, gamma);
}

void MiRaySceneLoader::loadCameraState(CameraState & state)
{
	state.reset(vec3(0.f), 0.f);

	auto flags = readUInt32();
	state.projection = (eCameraProjection)(flags & 7);
	if (state.projection < CP_PERSPECTIVE || state.projection > CP_FISHEYE)
		throw std::runtime_error("Invalid camera projection value.");

	state.depthOfField = flags & (1 << 8);

	seekChildrenChunks();

	while (m_pos < m_chunkEnd) {
		auto type = openChunk();
		switch (type) {
			case CT_CAMERA_POSITION:
				state.target = readVec3();
				break;

			case CT_CAMERA_ROTATION:
				state.yaw = readFloat();
				state.pitch = readFloat();
				state.roll = readFloat();
				break;

			case CT_CAMERA_DISTANCE:
				state.distance = readFloat();
				break;

			case CT_CAMERA_FOV:
				state.fov = readFloat();
				break;

			case CT_CAMERA_ASPECT:
				state.aspect = readFloat();
				break;

			case CT_CAMERA_Z_CLIP:
				state.nearZ = readFloat();
				state.farZ = readFloat();
				break;

			case CT_CAMERA_F_STOP:
				state.fStop = readFloat();
				break;

			case CT_CAMERA_FOCUS_DISTANCE:
				state.focusDistance = readFloat();
				break;

			case CT_CAMERA_DIAPHRAGM_BLADES:
				state.diaphragmBlades = readUInt32();
				break;

			case CT_CAMERA_BOKEH_ROTATION:
				state.bokehRotation = readFloat();
				break;

			case CT_CAMERA_GAMMA:
				state.gamma = readFloat();
				break;
		}
		closeChunk();
	}
}

void MiRaySceneLoader::loadCamera(Camera & camera)
{
	CameraState state;
	loadCameraState(state);
	camera._setState(state);
}

void MiRaySceneLoader::loadSnapshots(Scene &scene)
{
	auto numSnapshots = readUInt32();

	seekChildrenChunks();

	while (m_pos < m_chunkEnd) {
		auto type = openChunk();
		switch (type) {
			case CT_SNAPSHOT: {
				loadSnapshot(scene);
				break;
			}
		}
		closeChunk();
	}

	if (numSnapshots > 0)
		scene.snapshotManager().fireChanged();
}

void MiRaySceneLoader::loadSnapshot(Scene &scene)
{
	uint32_t flags;
	QString name;
	QString guid;
	if (m_context.fileVersion < 0x0103) {
		flags = SF_HAS_CAMERA_STATE;
		name = readStr();
	} else {
		flags = readUInt32();
		name = readStr();
		guid = readStr();
	}
	seekChildrenChunks();

	SnapshotPtr snapshot(new SnapshotImpl(scene, name));
	snapshot->hasCameraState()._set(flags & SF_HAS_CAMERA_STATE);
	snapshot->hasVisibility()._set(flags & SF_HAS_VISIBILITY);
	snapshot->hasTransformations()._set(flags & SF_HAS_TRANSFORMATIONS);
	snapshot->hasAssignedMaterials()._set(flags & SF_HAS_ASSIGNED_MATERIALS);
	snapshot->hasEnvironment()._set(flags & SF_HAS_ENVIRONMENT);
	snapshot->hasBackground()._set(flags & SF_HAS_BACKGROUND);
	if (!guid.isEmpty())
		snapshot->_setGuid(guid);
	bool valid = false;

	while (m_pos < m_chunkEnd) {
		auto type = openChunk();
		switch (type) {
			case 0xFF000604: // old snapshot state chunk
			case CT_SNAPSHOT_STATE: {
				QByteArray data(m_pos, (int)(m_dataEnd - m_pos));
				data = qUncompress(data);
				snapshot->_setState(QJsonDocument::fromJson(data).object());
				valid = true;
				break;
			}
		}
		closeChunk();
	}

	if (!valid) {
		throw std::runtime_error("Snapshot loading error.");
	}

	scene.snapshotManager()._add(snapshot, scene.snapshotManager().count());
}

void MiRaySceneLoader::loadMaterials()
{
	seekChildrenChunks();

	while (m_pos < m_chunkEnd) {
		auto type = openChunk();
		if (type == CT_MATERIAL)
			m_materials.push_back(std::make_tuple(m_pos, m_chunkEnd, m_dataEnd));
		closeChunk();
	}
}

void MiRaySceneLoader::loadMaterial(IMaterialCreator *materialCreator)
{
	QByteArray data(m_pos, (int)(m_dataEnd - m_pos));
	QJsonParseError parseError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);
	if (parseError.error == QJsonParseError::NoError && !jsonDoc.isNull() && jsonDoc.isObject()) {
		QJsonObject obj = jsonDoc.object();
		auto name = obj.value("name").toString(); // MATERIAL_NAME is "name"
		auto material = materialCreator->create(name, obj, m_context);
		if (!material)
			throw std::runtime_error("Unable to create material.");
		m_materialsMap[name] = material;
		return;
	}

	QDomDocument doc;
	if (!doc.setContent(data))
		throw std::runtime_error("Unable to read material.");

	auto nodeMaterial = doc.documentElement();
	auto name =  nodeMaterial.attribute("name"); // MATERIAL_NAME is "name"
	auto material = materialCreator->create(name, nodeMaterial, m_context);
	if (!material)
		throw std::runtime_error("Unable to create material.");

	m_materialsMap[name] = material;
}

void MiRaySceneLoader::loadRenderLayers(Scene & scene)
{
	seekChildrenChunks();

	m_renderLayers = { 0 };

	while (m_pos < m_chunkEnd) {
		auto type = openChunk();
		switch (type) {
			case CT_RENDER_LAYER: {
				auto name = readStr();
				auto renderLayer = scene.renderLayerManager().getByName(name);
				if (!renderLayer && m_context.loadNewScene) {
					renderLayer = scene.renderLayerManager()._create();
					renderLayer->name()._set(name);
				}
				m_renderLayers.emplace_back(renderLayer ? renderLayer->getIndex() : 0);
				break;
			}
		}
		closeChunk();
	}
}

INode * MiRaySceneLoader::loadChildNode(INode * node, eChunkType type)
{
	auto flags = readUInt32();
	mat4 mat;
	readData(&mat, sizeof(mat4));
	auto name = readStr();

	const eSceneElementType nodeTypes[] = {
		SceneElement_Node,
		SceneElement_MeshNode,
		SceneElement_Light,
		SceneElement_Node, // camera
		SceneElement_DirectionalLight,
		SceneElement_Unknown, // callout
		SceneElement_Unknown, // decal
	};
	auto child = static_cast<Node *>(node->addChildNode(name, nodeTypes[type - CT_NODE]));
	child->visible()._set(flags & 1);
	child->setTransformation(mat);
	if (m_context.fileVersion >= 0x0103 && m_context.loadNewScene)
		child->_setGuid(readStr());

	if (child->isMeshNode() && m_context.loadNewScene) {
		if (auto * meshNode = qobject_cast<MeshNode *>(child)) {
			int index = (flags >> 16) & 0xff; // render layer index
			meshNode->renderLayer()._setIndex(index < (int)m_renderLayers.size() ? m_renderLayers[index] : 0);
		}
	}

	loadNode(child);
	return child;
}
void MiRaySceneLoader::loadNode(Node * node)
{
	seekChildrenChunks();

	while (m_pos < m_chunkEnd) {
		auto type = openChunk();
		switch (type) {
			case CT_GEOMETRY:
				if (!node->isMeshNode())
					throw std::runtime_error("Invalid geometry chunk location.");

				if (auto meshNode = qobject_cast<IMeshNode *>(node))
					m_geometries.push_back(std::make_tuple(meshNode, m_pos, m_chunkEnd, m_dataEnd));
				else
					throw std::runtime_error("Mesh node reading error.");

				break;

			case CT_LIGHT:
				if (node->type() != SceneElement_Light)
					throw std::runtime_error("Invalid light chunk location.");

				if (auto lightNode = qobject_cast<LightNode *>(node))
					loadLight(lightNode);
				else
					throw std::runtime_error("Light reading error.");

				break;

			case CT_DIRECTIONAL_LIGHT:
				if (node->type() != SceneElement_DirectionalLight)
					throw std::runtime_error("Invalid direction light chunk location.");

				if (auto light = qobject_cast<DirectionalLight *>(node))
					loadDirectionalLight(light);
				else
					throw std::runtime_error("Direction light reading error.");

				break;

			case CT_NODE:
			case CT_MESH_NODE:
			case CT_LIGHT_NODE:
			case CT_DIRECTIONAL_LIGHT_NODE:
				loadChildNode(node, type);
				break;
		}
		closeChunk();
	}
}

bool UncompressData(char *dest, const char *src, size_t dataSize, size_t num, size_t elemSize, size_t pitch)
{
	auto data = qUncompress((uchar *)src, (int)dataSize);
	if ((size_t)data.size() != num * elemSize)
		return false;

	src = data.data();
	for (size_t i = 0; i < num; i++) {
		memcpy(dest, src, elemSize);
		dest += pitch;
		src += elemSize;
	}

	return true;
}

void MiRaySceneLoader::loadGeometry(IMeshNode * meshNode)
{
	readUInt32(); // flags
	auto numVertices = readUInt32();
	auto numIndices = readUInt32();

	auto materialName = readStr();
	auto it = m_materialsMap.find(materialName);
	auto material = it != m_materialsMap.end() ? it->second : m_defaultMaterial;

	QString guid;
	if (m_context.fileVersion >= 0x0103) guid = readStr();

	std::vector<Vertex> vertices(numVertices);
	std::vector<uint32_t> indices(numIndices);
	std::vector<vec2> uvSets[MAX_UV_SETS];
	const Vertex gv = { vec3(0.f), vec3(0.f, 0.f, 1.f) };
	std::fill(vertices.begin(), vertices.end(), gv);
	std::fill(indices.begin(), indices.end(), 0);

	seekChildrenChunks();
	while (m_pos < m_chunkEnd) {
		auto type = openChunk();
		auto dataSize = m_dataEnd - m_pos;
		switch (type) {
			case CT_GEOMETRY_POSITIONS:
				if (!UncompressData((char *)&vertices.data()->pos, m_pos, dataSize, numVertices, 3 * sizeof(float), sizeof(Vertex)))
					throw std::runtime_error("Unable to read geometry positions.");
				break;

			case CT_GEOMETRY_NORMALS:
				if (!UncompressData((char *)&vertices.data()->normal, m_pos, dataSize, numVertices, 3 * sizeof(float), sizeof(Vertex)))
					throw std::runtime_error("Unable to read geometry normals.");
				break;

			case CT_GEOMETRY_INDICES2:
				if (!UncompressData((char *)indices.data(), m_pos, dataSize, numIndices, 2, 4))
					throw std::runtime_error("Unable to read geometry indices.");
				break;

			case CT_GEOMETRY_INDICES4:
				if (!UncompressData((char *)indices.data(), m_pos, dataSize, numIndices, 4, 4))
					throw std::runtime_error("Unable to read geometry indices.");
				break;

			case CT_GEOMETRY_UV0:
				uvSets[0].resize(numVertices);
				if (!UncompressData((char *)uvSets[0].data(), m_pos, dataSize, numVertices, 2 * sizeof(float), sizeof(vec2)))
					throw std::runtime_error("Unable to read geometry uv set 0.");
				break;

			case CT_GEOMETRY_UV1:
				uvSets[1].resize(numVertices);
				if (!UncompressData((char *)uvSets[1].data(), m_pos, dataSize, numVertices, 2 * sizeof(float), sizeof(vec2)))
					throw std::runtime_error("Unable to read geometry uv set 1.");
				break;

			case CT_GEOMETRY_UV2:
				uvSets[2].resize(numVertices);
				if (!UncompressData((char *)uvSets[2].data(), m_pos, dataSize, numVertices, 2 * sizeof(float), sizeof(vec2)))
					throw std::runtime_error("Unable to read geometry uv set 2.");
				break;

			case CT_GEOMETRY_UV3:
				uvSets[3].resize(numVertices);
				if (!UncompressData((char *)uvSets[3].data(), m_pos, dataSize, numVertices, 2 * sizeof(float), sizeof(vec2)))
					throw std::runtime_error("Unable to read geometry uv set 3.");
				break;

		}
		closeChunk();
	}

	for (auto i : indices) {
		if (i >= numVertices)
			throw std::runtime_error("Invalid geometry indices.");
	}

	auto geom = static_cast<Geometry *>(meshNode->addGeometry(material));
	if (!guid.isEmpty() && m_context.loadNewScene)
		geom->_setGuid(guid);
	geom->setVertices(vertices);
	geom->setIndices(indices);
	for (int c = 0; c < MAX_UV_SETS; c++) {
		if (!uvSets[c].empty())
			geom->setUVset(c, uvSets[c]);
	}
}

void MiRaySceneLoader::loadLight(LightNode *light)
{
	//auto flags = ReadUInt32();
	seekChildrenChunks();

	while (m_pos < m_chunkEnd) {
		auto type = openChunk();
		switch (type) {
			case CT_LIGHT_COLOR:
				light->color()._set(readVec3());
				break;

			case CT_LIGHT_INTENSITY:
				light->intensity()._set(readIntensity());
				break;

			case CT_LIGHT_RADIUS:
				light->radius()._set(readFloat());
				break;
		}
		closeChunk();
	}
}

void MiRaySceneLoader::loadDirectionalLight(DirectionalLight *light)
{
	//auto flags = ReadUInt32();
	seekChildrenChunks();

	while (m_pos < m_chunkEnd) {
		auto type = openChunk();
		switch (type) {
			case CT_DIRECTIONAL_LIGHT_COLOR:
				light->color()._set(readVec3());
				break;

			case CT_LIGHT_INTENSITY:
			case CT_DIRECTIONAL_LIGHT_INTENSITY:
				light->intensity()._set(readIntensity() * 100.f);
				break;

			case CT_DIRECTIONAL_LIGHT_ANGULAR_SIZE:
				light->angularSize()._set(readFloat());
				break;
		}
		closeChunk();
	}
}

void MiRaySceneLoader::loadStep(const ModelLoadingContext &ctx, int si, IMaterialCreator *materialCreator)
{
	assert(m_context.sourceFileName == ctx.sourceFileName);

	if (si < (int)m_materials.size()) {
		auto matData = m_materials[si];
		m_pos = std::get<0>(matData);
		m_chunkEnd = std::get<1>(matData);
		m_dataEnd = std::get<2>(matData);
		m_chunks.push_back(m_chunkEnd);
		loadMaterial(materialCreator);
		m_chunks.pop_back();
		return;
	}

	si -= (int)m_materials.size();
	if (si < (int)m_geometries.size()) {
		auto geomData = m_geometries[si];
		m_pos = std::get<1>(geomData);
		m_chunkEnd = std::get<2>(geomData);
		m_dataEnd = std::get<3>(geomData);
		m_chunks.push_back(m_chunkEnd);
		loadGeometry(std::get<0>(geomData));
		m_chunks.pop_back();
		return;
	}

	throw std::runtime_error("Invalid loading step.");
}

// ------------------------------------------------------------------------ //

class SimpleMaterialCreator : public IMaterialCreator
{
	MaterialManager & m_materialManager;

public:
	SimpleMaterialCreator(MaterialManager & mm) : m_materialManager(mm) {}

	IMaterial *create(const QString &name, const QJsonObject & params, const ModelLoadingContext &ctx) override
	{
		return m_materialManager._create(name, params, ctx);
	}

	IMaterial *create(const QString &name, const QDomElement & data, const ModelLoadingContext &ctx) override
	{
		return m_materialManager._create(name, data, ctx);
	}
};

void MiRaySceneLoader::importNodes(INode *dest, IScene *sceneParam, const QByteArray &data)
{
	auto & scene = *static_cast<Scene *>(sceneParam);
	m_core = &scene.core();
	m_defaultMaterial = scene.materialManager().getDefault();
	m_pos = data.data();
	m_chunkEnd = m_pos + data.size();
	m_chunks.push_back(m_chunkEnd);

	m_context.sourceFileName.clear();
	m_context.sourceFolder.clear();
	m_context.loadNewScene = false;
	m_context.fileVersion = (VER_MAJOR << 8) | VER_MINOR;

	NodeSelection nodes;

	while (m_pos < m_chunkEnd) {
		auto type = openChunk();
		switch (type) {
			case CT_MATERIALS:
				loadMaterials();
				break;

			case CT_NODE:
			case CT_MESH_NODE:
			case CT_LIGHT_NODE:
			case CT_DIRECTIONAL_LIGHT_NODE:
				nodes.emplace_back(loadChildNode(dest, type));
				break;
		}
		closeChunk();
	}

	SimpleMaterialCreator materialCreator(scene.materialManager());
	auto numSteps = stepCount();
	for (int i = 0; i < numSteps; i++)
		loadStep(m_context, i, &materialCreator);

	{// update nodes transformation
		BBox bbox;
		bbox.clear();
		for (auto node : nodes) {
			static_cast<Node *>(node)->updateBoundingBox();
			auto childBBox = node->oobb();
			if (!childBBox.isNull()) {
				childBBox.transform(node->transformation());
				bbox.addToBounds(childBBox);
			}
		}

		if (!bbox.isNull()) {
			auto mat = dest->globalTransformation();
			vec3 offset((bbox.min.x + bbox.max.x) * 0.5f, (bbox.min.y + bbox.max.y) * 0.5, bbox.min.z);
			if (dest != &scene.root()) {
				auto oobb = dest->oobb();
				if (oobb.isNull())
					oobb = BBox(vec3(0.f), vec3(0.f));
				oobb.transform(mat);
				vec3 offset2((oobb.min.x + oobb.max.x) * 0.5f, (oobb.min.y + oobb.max.y) * 0.5, oobb.min.z);
				offset += glm::translation(mat) - offset2;
			}

			glm::setTranslation(mat, offset);
			mat = glm::inverse(mat);

			for (auto node : nodes)
				node->setTransformation(mat * node->transformation());
		}
	}

	scene.setSelection(Selection(nodes.begin(), nodes.end()), SelectionOperation_Set);
}
