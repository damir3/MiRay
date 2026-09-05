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

#include "Camera.h"
#include "Node.h"
#include "MeshNode.h"
#include "LightNode.h"
#include "DirectionalLight.h"
#include "SnapshotManager.h"
#include "RenderLayerManager.h"
#include "TextureParametersImpl.h"
#include "Materials/MaterialManager.h"

class IApplicationContext;
class CoreInstance;
class LightNode;

enum eSceneInternalModification {
	SceneInternalModification_Parameter,
	SceneInternalModification_Transformation,
	SceneInternalModification_Geometries,
	SceneInternalModification_Pivot,
	SceneInternalModification_Camera,
	SceneInternalModification_Material,
	SceneInternalModification_Texture,
	SceneInternalModification_Import,
	SceneInternalModification_Export,
	SceneInternalModification_Animation,
	SceneInternalModification_RenderLayers,
};

class Scene final : public IScene, public IParameterOwner, public ISceneProperties
{
	Q_OBJECT

	IApplicationContext &	m_ctx;
	CoreInstance &			m_core;
	MaterialManager 		m_materialManager;
	SnapshotManager			m_snapshotManager;
	RenderLayerManager 		m_renderLayerManager;

	QJsonObject				m_metadata;
	QMutex					m_mutex;
	std::atomic<int>		m_lockCount;
	RTCDevice				m_rtcDevice;
	RTCScene				m_rtcScene;
	Camera					m_camera;
	Node					m_root;
	SceneSphere				m_sceneSphere;
	int						m_colorCount;
	Selection				m_selection;
	QUndoStack				m_undo;

	EnumParameterImpl			m_backgroundMode;
	TextureColorParameterImpl	m_background;
	ColorParameterImpl			m_backgroundColor2;
	TextureColorParameterImpl	m_environment;
	ColorIntensityParameter		m_environmentIntensity;
	ScalarParameterImpl			m_environmentSize;
	ScalarParameterImpl			m_environmentVerticalOffset;
	ScalarParameterImpl			m_environmentHorizontalRotation;
	ScalarParameterImpl			m_environmentVerticalRotation;
	SRGBColorIntensityParameter	m_diffuseIntensity;
	BooleanParameterImpl		m_floorEnabled;
	ScalarParameterImpl			m_floorReflectionLevel;
	RoughnessParameter			m_floorRoughness;
	ScalarParameterImpl			m_floorShadowLevel;

	struct {
		struct {
			TextureColorParameterImpl::TransitionState color;
			float intensity;
			float size;
			float verticalOffset;
			float horizontalRotation;
			float verticalRotation;
		} environment;

		struct {
			int mode;
			TextureColorParameterImpl::TransitionState color;
		} background;
	} m_state[3];

	bool		m_transitionEnabled;
	float		m_transitionTime[2];
	float		m_transitionExponent;

	static void embreeErrorFunc(void* userPtr, enum RTCError code, const char* str);

	void setDefaultProperties();
	void setSelection(ISceneElement * elem, eSelectionOperation op);

	void updateSceneSphere();
	void updateFloor();
	void onBackgroundModeChanged();
	void _setBackgroundState(const QJsonObject & state);
	void _setEnvironmentState(const QJsonObject & state);

public:
	Scene(IApplicationContext & ctx, CoreInstance & core);
	~Scene();

	void reset();

	QByteArray save() const;

	QMutex * mutex() { return &m_mutex; }
	bool isValid() const { return m_lockCount == 0; }

	const Camera & camera() const { return  m_camera; }
	Camera & camera() override { return  m_camera; }

	Node & root() override { return m_root; }
	const SceneSphere & sceneSphere() const { return m_sceneSphere; }
	RTCDevice rtcDevice() { return m_rtcDevice; }
	RTCScene rtcScene() { return m_rtcScene; }

	const QJsonObject & metadata() const override { return m_metadata; }
	void setMetadata(QJsonObject metadata) override { m_metadata = metadata; }

	ISceneProperties & properties() override { return *this; }
	MaterialManager & materialManager() override { return m_materialManager; }
	SnapshotManager & snapshotManager() override { return m_snapshotManager; }
	RenderLayerManager & renderLayerManager() override { return m_renderLayerManager; }

	EnumParameterImpl & backgroundMode() final override { return m_backgroundMode; }
	TextureColorParameterImpl & background() final override { return m_background; }
	ColorParameterImpl & backgroundColor2() final override { return m_backgroundColor2; }
	TextureColorParameterImpl & environment() final override { return m_environment; }
	ColorIntensityParameter & environmentIntensity() final override { return m_environmentIntensity; }
	ScalarParameterImpl & environmentSize() final override { return m_environmentSize; }
	ScalarParameterImpl & environmentVerticalOffset() final override { return m_environmentVerticalOffset; }
	ScalarParameterImpl & environmentHorizontalRotation() final override { return m_environmentHorizontalRotation; }
	ScalarParameterImpl & environmentVerticalRotation() final override { return m_environmentVerticalRotation; }
	SRGBColorIntensityParameter & diffuseIntensity() final override { return m_diffuseIntensity; }
	BooleanParameterImpl & floorEnabled() final override { return m_floorEnabled; }
	ScalarParameterImpl & floorReflectionLevel() final override { return m_floorReflectionLevel; }
	RoughnessParameter & floorRoughness() final override { return m_floorRoughness; }
	ScalarParameterImpl & floorShadowLevel() final override { return m_floorShadowLevel; }

	QJsonObject getState(bool fullInfo);
	void _setState(const QJsonObject & state, uint32_t flags);
	void setState(const QJsonObject & state, uint32_t flags);
	void setAnimationState(const QJsonObject & state1, const QJsonObject & state2, uint32_t flags);
	void setTransitionFrame(float frameStart, float frameEnd, float exponent);
	void updateAnimation(int frame);
	void storeTransitionState(int i);
	void loadTransitionState(int i);

	QImage getPreview();

	void buildCollisionScene() override;

	vec3 getFrustumPosition(float x, float y, float z) const;

	void deleteElements(const Selection &elements) override;
	void setVisible(const NodeSelection &nodes, bool visible) override;

	void setSelection(const NodeSelection &selection, eSelectionOperation op);
	void setSelection(const Selection &selection, eSelectionOperation op) final override;
	bool setSelection(float x, float y, eSelectionOperation op);
	const Selection & selection() const final override { return m_selection; }
	NodeSelection nodeSelection() const final override;
	GeomSelection geomSelection() const final override;
	bool isSelected(const ISceneElement * elem) const override;
	bool isNodeGeomSelected(INode * node) const;

	void moveNodes(const NodeSelection &nodes, INode *target, size_t pos, const QString &groupName) override;
	INode *moveMeshes(const GeomSelection &geoms, bool deleteEmptyNodes, INode *targetNode) override;
	IGeometry *mergeMeshes(const GeomSelection &geoms) override;

	void setMaterial(const GeomSelection & geoms, IMaterial * material) override;
	void replaceMaterial(IMaterial * oldMaterial, IMaterial * newMaterial) override;
	bool isMaterialUsed(const IMaterial * material) const;

	void collectFileNames(QSet<QString> &res) const override;
	void updateFileNames(const QMap<QString, QString>&) override;

	ILightNode * addLight() override;
	IDirectionalLight * addDirectionalLight() override;

	QByteArray copyNodes(const NodeSelection &nodes) override;

	std::unique_ptr<IUVMapping> beginUVMapping(const GeomSelection &meshes) override;
	std::unique_ptr<IEditNormals> beginEditNormals(const GeomSelection &meshes) override;
	std::unique_ptr<IPivotParameters> beginPivotParameters(const NodeSelection &nodes) override;
	std::unique_ptr<ISnapshotInterpolator> beginSnapshotInterpolator(const ISnapshot & snapshot1, const ISnapshot & snapshot2) override;
	void beginDropToSurface(const NodeSelection &nodes) override;

	QString createGuid();
	vec3 generateUniqueColor();

	QUndoStack	&undoStack() override { return m_undo; }

	IApplicationContext & context() const { return m_ctx; }

	CoreInstance & core() const override { return m_core; }
	void pushCommand(QUndoCommand *cmd) override { m_undo.push(cmd); }
	void fireChanged(eParamId paramId) override;

	void fireCameraChanged();
	void fireSelectionChanged();
	void fireNodeChanged(const INode * node, eNodeChanged what);
	void fireBeforeSceneUpdate(const SceneModification &mod);
	void fireAfterSceneUpdate(const SceneModification &mod);
	void lock(eSceneInternalModification sm);
	void unlock(eSceneInternalModification sm);

signals:
	void cameraChanged();
	void sceneLock(eSceneInternalModification sm);
	void sceneUnlock(eSceneInternalModification sm);
};
