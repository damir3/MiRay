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

#include "Scene.h"

#include "CoreRenderer.h"
#include "SnapshotInterpolator.h"
#include "Samplers/Sobol.h"
#include "Samplers/Halton.h"
#include "Tools/UVMapping.h"
#include "Tools/EditNormals.h"
#include "Tools/PivotParameters.h"

#include "../../Shared/Interfaces/ModelSaver.h"
#include "../../Shared/Interfaces/SerializationContext.h"

#if !defined(__aarch64__) && !defined(_M_ARM64) && !defined(__arm64__)
#include "xmmintrin.h"
#include "pmmintrin.h"
#endif

static const char * const BACKGROUND_MODES[] = {
	"Environment",
	"Transparent",
	"Plain background",
	"Spherical background",
	"Radial gradient",
	"Vertical gradient",
	"Horizontal gradient",
	nullptr
};

FileFormatsList convertImageManagerFormatsToFilePicker(const ImageLoadingFormats& fmts);

// ------------------------------------------------------------------------ //

Scene::Scene(IApplicationContext & ctx, CoreInstance & core)
	: m_ctx(ctx)
	, m_core(core)
	, m_mutex(QMutex::Recursive)
	, m_rtcScene(nullptr)
	, m_camera(*this)
	, m_materialManager(*this)
	, m_snapshotManager(*this)
	, m_renderLayerManager(*this)
	, m_root("Root", *this, nullptr)
	, m_backgroundMode(BackgroundMode_PlaneImage, BACKGROUND_MODES, PID_SCENE_BACKGROUND_MODE, *this)
	, m_background(vec3(1.f), PID_SCENE_BACKGROUND, *this)
	, m_backgroundColor2(vec3(0.5f), PID_SCENE_BACKGROUND_COLOR2, *this)
	, m_environment(vec3(1.f), PID_SCENE_ENVIRONMENT, *this)
	, m_environmentIntensity(100.f, 0.f, FLT_MAX, 2, PID_SCENE_ENVIRONMENT_INTENSITY, *this)
	, m_environmentSize(1000.f, 0.f, FLT_MAX, 1, PID_SCENE_ENVIRONMENT_SIZE, *this)
	, m_environmentVerticalOffset(0.f, -1.f, 1.f, 3, PID_SCENE_ENVIRONMENT_VERTICAL_OFFSET, *this)
	, m_environmentHorizontalRotation(0.f, -360.f, 360.f, 1, PID_SCENE_ENVIRONMENT_HORIZONTAL_ROTATION, *this)
	, m_environmentVerticalRotation(0.f, -180.f, 180.f, 1, PID_SCENE_ENVIRONMENT_VERTICAL_ROTATION, *this)
	, m_diffuseIntensity(100.f, 0.f, 100.f, 1, PID_SCENE_DIFFUSE_INTENSITY, *this)
	, m_floorEnabled(false, PID_SCENE_FLOOR_ENABLED, *this)
	, m_floorReflectionLevel(0.f, 0.f, 100.f, 1, PID_SCENE_FLOOR_REFLECTION_LEVEL, *this, 0.01f)
	, m_floorRoughness(20.f, PID_SCENE_FLOOR_ROUGHNESS, *this)
	, m_floorShadowLevel(100.f, 0.f, FLT_MAX, 1, PID_SCENE_FLOOR_SHADOW_LEVEL, *this, 0.01f)
	, m_lockCount(0)
	, m_colorCount(0)
	, m_transitionEnabled(false)
{
	m_floorEnabled.setVisible(false);
	m_floorReflectionLevel.setVisible(false);
	m_floorRoughness.setVisible(false);
	m_floorShadowLevel.setVisible(false);
	m_background.texture()->mapping().setVisible(false);
	m_background.texture()->mapping().setEnabled(false);
	m_environment.texture()->mapping().setVisible(false);
	m_environment.texture()->mapping().setEnabled(false);

#if !defined(__aarch64__) && !defined(_M_ARM64) && !defined(__arm64__)
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
	_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif

	m_rtcDevice = rtcNewDevice("verbose=1");

	/* Sets the error callback function. */
	rtcSetDeviceErrorFunction(m_rtcDevice, embreeErrorFunc, this);
	m_rtcScene = rtcNewScene(m_rtcDevice);
	rtcSetSceneBuildQuality(m_rtcScene, RTC_BUILD_QUALITY_HIGH);
	rtcCommitScene(m_rtcScene);
	updateSceneSphere();
	setDefaultProperties();
}

Scene::~Scene()
{
	m_undo.clear();
	m_root.destroyCollisionGeometries();
	rtcReleaseScene(m_rtcScene);
	rtcReleaseDevice(m_rtcDevice);
}

// ------------------------------------------------------------------------ //

void Scene::embreeErrorFunc(void* userPtr, enum RTCError code, const char* message)
{
	static const char * errors[] = {
		"none",
		"unknown",
		"invalid argument",
		"invalid operation",
		"out of memory",
		"unsupported CPU",
		"cancelled"
	};
	LogError() << QString().asprintf("Embree (%s) error: %s", errors[code], message ? message : "");
}

// ------------------------------------------------------------------------ //

void Scene::reset()
{
	m_root.destroyCollisionGeometries();
	m_root.removeChildren();
	m_root.updateBoundingBox();
	m_camera.reset();

	m_materialManager.reset();
	m_snapshotManager.reset();

	rtcReleaseScene(m_rtcScene);
	m_rtcScene = rtcNewScene(m_rtcDevice);

	setDefaultProperties();
}

void Scene::setDefaultProperties()
{
	m_backgroundMode._setIndex(BackgroundMode_Environment);
	onBackgroundModeChanged();

	m_background._set(vec3(1.f));
	m_background.texture()->reset(QString());
	m_background.texture()->wrapX()._setIndex(ITexture::WRAP_CLAMP);
	m_background.texture()->wrapY()._setIndex(ITexture::WRAP_CLAMP);
	m_background.setEnabled(false);

	m_environment._set(vec3(1.f));
	m_environment.texture()->reset("miray://library/Environments/Studio/story_studio_01_2k.hdr");
	m_environment.texture()->setEnabledMappingParams(false);

	// reset floor settings
	m_floorEnabled._set(false);
	m_floorReflectionLevel._set(0.f);
	m_floorRoughness._set(20.f);
	m_floorShadowLevel._set(100.f);
	updateFloor();
}

// ------------------------------------------------------------------------ //

static const QString keyCamera("camera");
static const QString keyEnvironment("environment");
static const QString keyBackground("background");
static const QString keyNodes("nodes");
static const QString keyGeometries("geometries");

QJsonObject Scene::getState(bool fullInfo)
{
	QJsonObject environment = m_environment.getState();
	environment["intensity"] = m_environmentIntensity.get();
	environment["size"] = m_environmentSize.get();
	environment["vertical-offset"] = m_environmentVerticalOffset.get();
	environment["horizontal-rotation"] = m_environmentHorizontalRotation.get();
	environment["vertical-rotation"] = m_environmentVerticalRotation.get();

	QJsonObject background = m_background.getState();
	background["mode"] = m_backgroundMode.getIndex();
	background["color2"] = toJsonArray(m_backgroundColor2.get());

	QJsonObject nodes;
	QJsonObject geoms;
	m_root.getStateRecursive(nodes, geoms, fullInfo);

	return QJsonObject {
		{ keyCamera, m_camera.getState(fullInfo) },
		{ keyEnvironment, environment },
		{ keyBackground, background },
		{ keyNodes, nodes },
		{ keyGeometries, geoms }
	};
}

void Scene::_setBackgroundState(const QJsonObject & background)
{
	m_background.setState(background);
	m_backgroundMode._setIndex(background.value("mode").toInt(m_backgroundMode.getIndex()));
	m_backgroundColor2._set(getVec3(background, "color2", m_backgroundColor2.get()));
	onBackgroundModeChanged();
}

void Scene::_setEnvironmentState(const QJsonObject & environment)
{
	m_environment.setState(environment);
	m_environmentIntensity._set(environment.value("intensity").toDouble(m_environmentIntensity.get()));
	m_environmentSize._set(environment.value("size").toDouble(m_environmentSize.get()));
	m_environmentVerticalOffset._set(environment.value("vertical-offset").toDouble(m_environmentVerticalOffset.get()));
	m_environmentHorizontalRotation._set(environment.value("horizontal-rotation").toDouble(m_environmentHorizontalRotation.get()));
	m_environmentVerticalRotation._set(environment.value("vertical-rotation").toDouble(m_environmentVerticalRotation.get()));
}

void Scene::_setState(const QJsonObject & state, uint32_t flags)
{
	lock(SceneInternalModification_Import);

	m_transitionEnabled = false;

	if ((flags & SF_HAS_CAMERA_STATE) && state.contains(keyCamera))
		m_camera._setState(state[keyCamera].toObject());

	if ((flags & SF_HAS_ENVIRONMENT) && state.contains(keyEnvironment))
		_setEnvironmentState(state[keyEnvironment].toObject());

	if ((flags & SF_HAS_BACKGROUND) && state.contains(keyBackground))
		_setBackgroundState(state[keyBackground].toObject());

	if (flags & (SF_HAS_VISIBILITY | SF_HAS_TRANSFORMATIONS | SF_HAS_ASSIGNED_MATERIALS))
		m_root.setStateRecursive(state[keyNodes].toObject(), state[keyGeometries].toObject(), flags);

	unlock(SceneInternalModification_Import);
}

void Scene::setState(const QJsonObject & map, uint32_t flags)
{
	sceneChangeState(*this, getState(true), map, flags);
}

void Scene::storeTransitionState(int i)
{
	auto & state = m_state[i];

	m_environment.getTransitionState(state.environment.color);
	state.environment.intensity = m_environmentIntensity.get();
	state.environment.size = m_environmentSize.get();
	state.environment.verticalOffset = m_environmentVerticalOffset.get();
	state.environment.horizontalRotation = m_environmentHorizontalRotation.get();
	state.environment.verticalRotation = m_environmentVerticalRotation.get();

	m_background.getTransitionState(state.background.color);
	state.background.mode = m_backgroundMode.getIndex();
}

void Scene::loadTransitionState(int i)
{
	const auto & state = m_state[i];

	m_environment.setTransitionState(state.environment.color);
	m_environmentIntensity._set(state.environment.intensity);
	m_environmentSize._set(state.environment.size);
	m_environmentVerticalOffset._set(state.environment.verticalOffset);
	m_environmentHorizontalRotation._set(state.environment.horizontalRotation);
	m_environmentVerticalRotation._set(state.environment.verticalRotation);

	m_background.setTransitionState(state.background.color);
	m_backgroundMode._setIndex(state.background.mode);
}

void Scene::setAnimationState(const QJsonObject & state1, const QJsonObject & state2, uint32_t flags)
{
	lock(SceneInternalModification_Import);

	m_transitionEnabled = false;

	m_camera.setAnimationState(state1[keyCamera].toObject(), state2[keyCamera].toObject(), flags);

	if (flags & SF_HAS_ENVIRONMENT)
		_setEnvironmentState(state1[keyEnvironment].toObject());

	if (flags & SF_HAS_BACKGROUND)
		_setBackgroundState(state1[keyBackground].toObject());

	storeTransitionState(0);

	if (flags & SF_HAS_ENVIRONMENT)
		_setEnvironmentState(state2[keyEnvironment].toObject());

	if (flags & SF_HAS_BACKGROUND)
		_setBackgroundState(state2[keyBackground].toObject());

	storeTransitionState(1);

	m_root.setAnimationStateRecursive(state1[keyNodes].toObject(), state2[keyNodes].toObject(), state1[keyGeometries].toObject(), state2[keyGeometries].toObject(), flags);

	unlock(SceneInternalModification_Import);
}

void Scene::setTransitionFrame(float frameStart, float frameEnd, float exponent)
{
	lock(SceneInternalModification_Import);

	m_transitionEnabled = true;
	m_transitionTime[0] = std::clamp(frameStart, 0.f, 1.f);
	m_transitionTime[1] = std::clamp(frameEnd, 0.f, 1.f);
	m_transitionExponent = exponent;

	unlock(SceneInternalModification_Import);
}

class AnimationSampler final : public Sampler
{
	int m_index;
	const int m_frame;
	SobolSampler	m_sampler;

	void updateIndex()
	{
		m_sampler.init(m_index & 0xF, m_index >> 4, m_frame);
		if (++m_index == 256) m_index = 1;
	}

public:

	AnimationSampler(int frame) : m_index(0), m_frame(frame)
	{
		m_sampler.setResolution(16, 16);
	}

	float generate1D()
	{
		updateIndex();
		return m_sampler.generate1D();
	}

	vec2 generate2D()
	{
		updateIndex();
		return m_sampler.generate2D();
	}

	vec3 generate3D()
	{
		updateIndex();
		return m_sampler.generate3D();
	}
};

void Scene::updateAnimation(int frame)
{
	if (!m_transitionEnabled)
		return;

	AnimationSampler sampler(frame);

	auto sample = sampler.generate2D();

	float t = lerp(m_transitionTime[0], m_transitionTime[1], sample.x);
	t = t < 0.5f ? std::pow(t * 2.f, m_transitionExponent) * 0.5f : 1.f - std::pow((1 - t) * 2.f, m_transitionExponent) * 0.5f;

	lock(SceneInternalModification_Animation);

	m_camera.setAnimationTime(t);

	loadTransitionState(sample.y > t ? 0 : 1);

	m_root.setAnimationTimeRecursive(t, sampler);

	unlock(SceneInternalModification_Animation);
}

// ------------------------------------------------------------------------ //

QImage Scene::getPreview()
{
	return QImage();
}

// ------------------------------------------------------------------------ //

void Scene::buildCollisionScene()
{
	m_root.updateBoundingBox();
	m_root.createCollisionGeometries();
}

void Scene::updateSceneSphere()
{
	const auto & bbox = m_root.oobb();
	if (!bbox.isNull()) {
		m_sceneSphere.center = bbox.center();
		m_sceneSphere.radius = glm::length(bbox.size()) * 0.5f;
		m_sceneSphere.invRadiusSqr = 1.f / sqr(m_sceneSphere.radius);
	} else {
		m_sceneSphere.center = vec3(0.f);
		m_sceneSphere.radius = 1.f;
		m_sceneSphere.invRadiusSqr = 1.f;
	}
}

void Scene::updateFloor()
{
	auto enabled = m_floorEnabled.get();
	m_floorReflectionLevel.setEnabled(enabled);
	m_floorReflectionLevel.setVisible(enabled);
	m_floorRoughness.setEnabled(enabled && m_floorReflectionLevel.value() > 0.f);
	m_floorRoughness.setVisible(enabled);
	m_floorShadowLevel.setEnabled(enabled && m_floorReflectionLevel.value() < 1.f);
	m_floorShadowLevel.setVisible(enabled);
}

// ------------------------------------------------------------------------ //

vec3 Scene::getFrustumPosition(float x, float y, float z) const
{
	auto v = glm::inverse(m_camera.viewProjMatrix()) * vec4(x, y, z, 1.f);
	return vec3(v.x, v.y, v.z) / v.w;
}

// ------------------------------------------------------------------------ //

void Scene::setSelection(ISceneElement * elem, eSelectionOperation op)
{
	switch (op) {
		case SelectionOperation_Set:
			m_selection.clear();
			m_selection.insert(elem);
			if (elem->type() == SceneElement_Geometry)
				m_selection.insert(elem->parent());
			break;
		case SelectionOperation_Add:
			if (isSelected(elem)) return;
			m_selection.insert(elem);
			if (elem->type() == SceneElement_Geometry)
				m_selection.insert(elem->parent());
			break;
		case SelectionOperation_Remove:
			if (!isSelected(elem)) return;
			m_selection.erase(elem);
			if (elem->type() == SceneElement_Geometry && !isNodeGeomSelected(elem->parent()))
				m_selection.erase(elem->parent());
			break;
		case SelectionOperation_Invert:
			if (isSelected(elem)) {
				m_selection.erase(elem);
				if (elem->type() == SceneElement_Geometry && !isNodeGeomSelected(elem->parent()))
					m_selection.erase(elem->parent());
			} else {
				m_selection.insert(elem);
				if (elem->type() == SceneElement_Geometry)
					m_selection.insert(elem->parent());
			}
			break;
	}

	emit selectionChanged();
}

void Scene::setSelection(const Selection &selection, eSelectionOperation op)
{
	assert(op == SelectionOperation_Set); // TODO: add more

	m_selection = selection;

	emit selectionChanged();
}

void Scene::setSelection(const NodeSelection &selection, eSelectionOperation op)
{
	assert(op == SelectionOperation_Set); // TODO: add more

	m_selection.clear();
	for (auto node : selection)
		m_selection.insert(node);

	emit selectionChanged();
}

bool Scene::setSelection(float x, float y, eSelectionOperation op)
{
	if (m_lockCount > 0)
		return false;

	Ray ray;
	ray.init(getFrustumPosition(x, y, -1.f), getFrustumPosition(x, y, 1.f));
	rtcIntersect1(m_rtcScene, &ray);

	if (ray.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
		if (ray.hit.primID == RTC_INVALID_GEOMETRY_ID) { // light
			if (auto node = LightNode::getByRay(m_rtcScene, ray)) {
				setSelection(node, op);
				return true;
			}
		} else {// geometry
			if (auto node = MeshNode::getByRay(m_rtcScene, ray)) {
				if (auto geom = node->getGeometryByRay(ray)) {
					setSelection(geom, op);
					return true;
				}
			}
		}
	}

	if (op == SelectionOperation_Set) {
		m_selection.clear();
		emit selectionChanged();
	}

	return false;
}

NodeSelection Scene::nodeSelection() const
{
	NodeSelection nodes;
	for (auto elem : m_selection) {
		if (elem->type() != SceneElement_Geometry) {
			if (auto node = static_cast<INode *>(elem))
				nodes.push_back(node);
		}
	}

	return nodes;
}

GeomSelection Scene::geomSelection() const
{
	GeomSelection geoms;
	for (auto elem : m_selection) {
		if (elem->type() == SceneElement_Geometry && elem->parent()->type() == SceneElement_MeshNode) {
			if (auto geom = static_cast<IGeometry *>(elem))
				geoms.push_back(geom);
		}
	}

	return geoms;
}

bool Scene::isSelected(const ISceneElement * elem) const
{
	return m_selection.contains(const_cast<ISceneElement *>(elem));
}

bool Scene::isNodeGeomSelected(INode * node) const
{
	if (static_cast<const Node *>(node)->isMeshNode()) {
		if (auto meshNode = qobject_cast<const MeshNode *>(node)) {
			for (const auto & geom : meshNode->geometries()) {
				if (isSelected(geom.get()))
					return true;
			}
		}
	}

	return false;
}

// ------------------------------------------------------------------------ //

void Scene::moveNodes(const NodeSelection &nodes, INode *target, size_t pos, const QString &groupName)
{
	sceneMoveNodes(*this, target ? static_cast<Node *>(target) : &m_root, pos, groupName, nodes);
}

INode *Scene::moveMeshes(const GeomSelection &geoms, bool deleteEmptyNodes, INode *targetNode)
{
	auto moveMeshesCommand = new MoveMeshesCommand(geoms, deleteEmptyNodes, targetNode ? static_cast<Node *>(targetNode) : &m_root);
	pushCommand(moveMeshesCommand);
	return moveMeshesCommand->getTargetNode();
}

// ------------------------------------------------------------------------ //

void Scene::deleteElements(const Selection &elements)
{
	sceneDeleteElements(*this, elements);
}

void Scene::setVisible(const NodeSelection &nodes, bool visible)
{
	sceneShowNodes(*this, nodes, visible);
}

// ------------------------------------------------------------------------ //

void Scene::setMaterial(const GeomSelection & geometries, IMaterial * mat)
{
	auto material = static_cast<MaterialImpl *>(mat);
	if (&material->owner() != &m_materialManager) {
		auto data = material->save(ModelSavingContext());
		m_undo.beginMacro("Set Material");
		material = m_materialManager.load(data, ModelLoadingContext(), m_materialManager.count());
		sceneSetMaterial(*this, geometries, material);
		m_undo.endMacro();
	} else
		sceneSetMaterial(*this, geometries, material);
}

void Scene::replaceMaterial(IMaterial * oldMaterial, IMaterial * newMaterial)
{
	std::vector<IGeometry *> geometries;
	m_root.findGeometriesByMaterial(geometries, oldMaterial);
	if (geometries.empty())
		return;

	auto material = static_cast<MaterialImpl *>(newMaterial);
	if (&material->owner() != &m_materialManager) {
		auto data = material->save(ModelSavingContext());
		m_undo.beginMacro("Replace Material");
		newMaterial = m_materialManager.load(data, ModelLoadingContext(), m_materialManager.count());
		sceneSetMaterial(*this, geometries, newMaterial);
		m_undo.endMacro();
	} else
		sceneSetMaterial(*this, geometries, newMaterial);
}

bool Scene::isMaterialUsed(const IMaterial * material) const
{
	if (m_root.isMaterialUsed(material))
		return true;

	for (size_t i = 0; i < m_snapshotManager.count(); ++i) {
		const auto * snapshot = m_snapshotManager.get(i);
		const auto & state = snapshot->getState();
		const auto geometryStates = state[keyGeometries].toObject();
		for (auto it = geometryStates.constBegin(); it != geometryStates.constEnd(); ++it) {
			const auto & geomState = it.value().toObject();
			if (geomState["m"].toString() == material->name().get())
				return true;
		}
	}

	return false;
}

void Scene::collectFileNames(QSet<QString> &res) const
{
	res.clear();

	res.insert(TextureImpl::safeGetFileName(m_background.texture()));
	res.insert(TextureImpl::safeGetFileName(m_environment.texture()));
	m_materialManager.collectFileNames(res);

	res.remove(QString());
}

void Scene::updateFileNames(const QMap<QString, QString>& map)
{
	if (map.isEmpty())
		return;

	QMap<QString, QString>	good;
	for (auto it = map.begin(); it != map.end(); it++)
		good[nativePath(it.key())] = it.value();

	try {
		m_undo.beginMacro("Update File Names");
		TextureImpl::updateFileNames(good, m_background.texture());
		TextureImpl::updateFileNames(good, m_environment.texture());

		m_materialManager.updateFileNames(good);
		m_undo.endMacro();
	} catch (const std::exception &) {
		// rollback transaction
		m_undo.endMacro();
		m_undo.undo();

		// and throw it further
		throw;
	}
}

// ------------------------------------------------------------------------ //

ILightNode * Scene::addLight()
{
	auto light = qobject_cast<LightNode *>(m_root.addChildNode("Light", SceneElement_Light));
	if (!light)
		return nullptr;

	NodePtr lightPtr = m_root.removeChildNode(m_root.children().size() - 1);

	auto & bbox = m_root.oobb();
	vec3 target;
	float dist;
	if (bbox.isNull()) {
		target = vec3(0.f);
		dist = 10.f;
	} else {
		target = bbox.center();
		dist = std::max(glm::length(bbox.size()) * 0.5f, 1.f);
	}

	std::vector<AbstractLight *> lights;
	m_root.findLights(lights, false);
	auto yaw = ((lights.size() + 1) / 2) * 30.f;
	if (lights.size() % 2) yaw = -yaw;
	light->intensity()._set(sqr(dist) * 4.f);
	light->radius()._set(dist * 0.1f);
	light->setTransformation(MatrixBuilder().AddPosition(target + anglesToVector(vec3(0.f, 50.f, yaw - 90.f)) * dist * 2.f));

	sceneCreateNode(*this, lightPtr);

	return light;
}

IDirectionalLight * Scene::addDirectionalLight()
{
	auto light = qobject_cast<DirectionalLight *>(m_root.addChildNode("Directional Light", SceneElement_DirectionalLight));
	if (!light)
		return nullptr;

	NodePtr lightPtr = m_root.removeChildNode(m_root.children().size() - 1);

	Random random;
	light->setTransformation(MatrixBuilder().AddRotation(vec3(0.f, 45.f + std::floor(random.generate1D() * 45.f), std::floor(random.generate1D() * -90.f))));

	sceneCreateNode(*this, lightPtr);

	return light;
}

// ------------------------------------------------------------------------ //

QByteArray Scene::copyNodes(const NodeSelection &nodes)
{
	NodeSelection uniqueNodes;
	uniqueNodes.reserve(nodes.size());
	for (auto node : nodes) {
		bool skipNode = false;
		for (auto checkNode : uniqueNodes) {
			if (checkNode == node || static_cast<Node *>(checkNode)->hasChild(node)) {
				skipNode = true;
				break;
			}
		}

		if (!skipNode)
			uniqueNodes.push_back(node);
	}

	auto savers = m_ctx.modelSaversRegistry();
	auto saver = savers->getSaver("data.mirayScene");

	return saver->exportNodes(uniqueNodes);
}

// ------------------------------------------------------------------------ //

std::unique_ptr<IUVMapping> Scene::beginUVMapping(const GeomSelection &meshes)
{
	return std::unique_ptr<IUVMapping>(new UVMapping(*this, meshes));
}

std::unique_ptr<IEditNormals> Scene::beginEditNormals(const GeomSelection &meshes)
{
	return std::unique_ptr<IEditNormals>(new EditNormals(*this, meshes));
}

std::unique_ptr<IPivotParameters> Scene::beginPivotParameters(const NodeSelection &nodes)
{
	return std::unique_ptr<IPivotParameters>(new PivotParameters(*this, nodes));
}

// ------------------------------------------------------------------------ //

std::unique_ptr<ISnapshotInterpolator> Scene::beginSnapshotInterpolator(const ISnapshot & snapshot1, const ISnapshot & snapshot2)
{
	return std::unique_ptr<ISnapshotInterpolator>(new SnapshotInterpolator(*this, snapshot1, snapshot2));
}

static bool hasParentInSelection(Node * node, const NodeSelection & nodes)
{
	for (auto * checkNode : nodes) {
		if (node != checkNode && node->hasParent(checkNode))
			return true;
	}
	return false;
}

void Scene::beginDropToSurface(const NodeSelection &nodes)
{
	assert(!nodes.empty());
	if (nodes.empty())
		return;

	NodeSelection rootNodes;
	for (auto * node : nodes) {
		if (!hasParentInSelection(static_cast<Node *>(node), nodes))
			rootNodes.push_back(node);
	}

	if (rootNodes.empty())
		return;

	std::sort(rootNodes.begin(), rootNodes.end(), [](INode * a, INode * b) {
		return a->getAABB().min.z < b->getAABB().min.z;
	});

	auto cmd = new TransformationCommand(*this, rootNodes, true);

	rtcCommitScene(m_rtcScene);

	for (auto * node : rootNodes) {
		BBox bbox = node->getAABB();
		vec3 bottomPoint = node->getObjectBottomPoint(bbox);

		float hitZ = 0.f;

		if (bottomPoint.z > 0.f) {
			Ray ray;
			ray.init(bottomPoint, vec3(0.f, 0.f, -1.f), bottomPoint.z);
			ray.ray.tnear = 1e-4f;

			rtcIntersect1(m_rtcScene, &ray);

			while (ray.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
				Node * hitNode = nullptr;
				if (ray.hit.primID == RTC_INVALID_GEOMETRY_ID) {
					hitNode = LightNode::getByRay(m_rtcScene, ray);
				} else {
					hitNode = MeshNode::getByRay(m_rtcScene, ray);
				}

				bool isSelf = false;
				if (hitNode) {
					isSelf = (hitNode == node || hitNode->hasParent(node));
				}

				if (!isSelf) {
					hitZ = std::max(0.f, bottomPoint.z - ray.ray.tfar);
					break;
				}

				float nextTnear = ray.ray.tfar + 1e-4f;
				if (nextTnear >= bottomPoint.z) {
					break;
				}

				ray.ray.tnear = nextTnear;
				ray.ray.tfar = bottomPoint.z;
				ray.resetHit();
				rtcIntersect1(m_rtcScene, &ray);
			}
		}

		float deltaZ = hitZ - bottomPoint.z;
		const auto mat = glm::translate(glm::mat4(1.f), vec3(0.f, 0.f, deltaZ));
		cmd->setNodeTransformation(node, mat);
	}

	pushCommand(cmd);
}

// ------------------------------------------------------------------------ //

QString Scene::createGuid()
{
	return QUuid::createUuid().toString().mid(1, 8);
}

vec3 Scene::generateUniqueColor()
{
	HaltonSampler sampler;
	sampler.setResolution(2, 2);
	sampler.init(0, 0, ++m_colorCount);
	return sampler.generate3D();
}

// ------------------------------------------------------------------------ //

void Scene::onBackgroundModeChanged()
{
	auto bgMode = m_backgroundMode.getIndex();
	m_background.setEnabled(bgMode >= BackgroundMode_PlaneImage);
	m_backgroundColor2.setEnabled(bgMode >= BackgroundMode_RadialGradient);
	m_background.texture()->setEnabledMappingParams(bgMode != BackgroundMode_Environment && bgMode != BackgroundMode_Transparent && bgMode != BackgroundMode_SphericalImage);
}

void Scene::fireChanged(eParamId paramId)
{
	switch (paramId) {
		case PID_SCENE_BACKGROUND_MODE:
			onBackgroundModeChanged();
			break;

		case PID_SCENE_FLOOR_ENABLED:
		case PID_SCENE_FLOOR_REFLECTION_LEVEL:
			updateFloor();
			break;
	}
}

void Scene::fireCameraChanged()
{
	emit cameraChanged();
}

void Scene::fireSelectionChanged()
{
	emit selectionChanged();
}

void Scene::fireNodeChanged(const INode * node, eNodeChanged what)
{
	emit nodeChanged(node, what);
}

void Scene::fireBeforeSceneUpdate(const SceneModification &mod)
{
	assert(m_lockCount > 0);
	emit beforeSceneUpdate(mod);
}

void Scene::fireAfterSceneUpdate(const SceneModification &mod)
{
	assert(m_lockCount > 0);
	emit afterSceneUpdate(mod);
}

void Scene::lock(eSceneInternalModification sm)
{
	if (m_lockCount++ == 0) {
		emit sceneLock(sm);
		m_mutex.lock();
	}
}

void Scene::unlock(eSceneInternalModification sm)
{
	if (sm == SceneInternalModification_Transformation ||
		sm == SceneInternalModification_Geometries ||
		sm == SceneInternalModification_Pivot ||
		sm == SceneInternalModification_Import ||
		sm == SceneInternalModification_Animation) {
		rtcCommitScene(m_rtcScene);
		updateSceneSphere();
	}

	if (sm == SceneInternalModification_Import) {
		updateFloor();
		m_background.setEnabled(m_backgroundMode.getIndex() == BackgroundMode_PlaneImage || m_backgroundMode.getIndex() == BackgroundMode_SphericalImage);
		emit completelyChanged();
	}

	assert(m_lockCount > 0);
	if (--m_lockCount == 0) {
		emit sceneUnlock(sm);
		m_mutex.unlock();
	}
}
