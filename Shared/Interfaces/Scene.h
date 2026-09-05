#pragma once

#include "Node.h"

class ISnapshot;
class IGeometry;
class IMaterial;

// scene metadata keys and default values

#define SCENE_METADATA_KEY_RENDERING_WIDTH						"RenderingWidth"
#define SCENE_METADATA_KEY_RENDERING_HEIGHT						"RenderingHeight"
#define SCENE_METADATA_KEY_RENDERING_PRESET						"RenderingPreset"
#define SCENE_METADATA_KEY_RENDERING_PRESET_DATA				"RenderingPresetData"
#define SCENE_METADATA_KEY_RENDERING_KEEP_RATIO					"RenderingRatio"
#define SCENE_METADATA_KEY_RENDERING_ADVANCED					"RenderingAdvanced"
#define SCENE_METADATA_KEY_RENDERING_CAUSTICS					"RenderingCaustics"
#define SCENE_METADATA_KEY_RENDERING_MAX_INTENSITY				"RenderingMaxIntensity"
#define SCENE_METADATA_KEY_RENDERING_PHOTON_SCALE				"RenderingPhotonScale"
#define SCENE_METADATA_KEY_RENDERING_EXTRA_CHANNELS				"RenderingExtraChannels"
#define SCENE_METADATA_KEY_RENDERING_RENDER_LATER_JOB_NAME		"RenderingLaterName"
#define SCENE_METADATA_KEY_RENDERING_RENDER_LATER_ENABLED		"RenderingLaterEnabled"
#define SCENE_METADATA_KEY_RENDERING_OUTPUT_IMAGE_FORMAT		"RenderingOutputFormat"
#define SCENE_METADATA_KEY_RENDERING_RENDER_EXTRA_CHANNELS		"RenderingExtraChannels"
#define SCENE_METADATA_KEY_RENDERING_DENOISE					"RenderingDenoise"

#define SCENE_METADATA_DEFAULT_RENDERING_WIDTH					512
#define SCENE_METADATA_DEFAULT_RENDERING_HEIGHT					512
#define SCENE_METADATA_DEFAULT_RENDERING_ITERATIONS				2000
#define SCENE_METADATA_DEFAULT_RENDERING_TIME					300	// sec
#define SCENE_METADATA_DEFAULT_RENDERING_KEEP_ASPECT_RATIO		true
#define SCENE_METADATA_DEFAULT_RENDERING_CAUSTICS				false
#define SCENE_METADATA_DEFAULT_RENDERING_EXTRA_CHANNELS			false
#define SCENE_METADATA_DEFAULT_RENDERING_DENOISE				false
#define SCENE_METADATA_DEFAULT_RENDERING_MAX_INTENSITY_LIMITED	4.f
#define SCENE_METADATA_DEFAULT_RENDERING_MAX_INTENSITY_FULL		100.f
#define SCENE_METADATA_DEFAULT_RENDERING_PHOTON_SCALE			1.f
#define SCENE_METADATA_DEFAULT_RENDERING_RENDER_LATER_JOB_NAME	""
#define SCENE_METADATA_DEFAULT_RENDERING_RENDER_LATER_ENABLED	false

// other stuff

enum eSelectionOperation {
	SelectionOperation_Set,
	SelectionOperation_Add,
	SelectionOperation_Remove,
	SelectionOperation_Invert,
};

enum eTransformationOperation {
	TransformationOperation_Translation,
	TransformationOperation_Rotation,
	TransformationOperation_Scale,
};

enum eNodeChanged {
	NodeChanged_ChildrenList	= (1 << 1),
	NodeChanged_GeometryList	= (1 << 2),
	NodeChanged_Transformation	= (1 << 3),
	NodeChanged_Visibility		= (1 << 4),
	NodeChanged_Name			= (1 << 5),
};

enum eSceneModification {
	SceneModification_Unknown,
	SceneModification_Add,
	SceneModification_Delete,
	SceneModification_Move,
	SceneModification_Visibility,
	SceneModification_Material,
};

enum eBackgroundMode {
	BackgroundMode_Environment,
	BackgroundMode_Transparent,
	BackgroundMode_PlaneImage,
	BackgroundMode_SphericalImage,
	BackgroundMode_RadialGradient,
	BackgroundMode_VerticalGradient,
	BackgroundMode_HorizontalGradient,
};

struct SceneModification { // use SceneModificationXXX() methods from Core/src/Utils.h to init this structure
	eSceneModification type = SceneModification_Unknown;
	ISceneElement * sourceElement = nullptr; // source node or geometry

	const INode * targetNode = nullptr; // target node for add/move, parent node for delete
	int insertPosition = 0; // target position index under targetNode
};

class ISceneProperties
{
protected:
	virtual ~ISceneProperties() {}

public:

	virtual IEnumParameter & backgroundMode() = 0;
	virtual IColorParameter & background() = 0;
	virtual IColorParameter & backgroundColor2() = 0;

	virtual IColorParameter & environment() = 0;
	virtual IScalarParameter & environmentIntensity() = 0;
	virtual IScalarParameter & environmentSize() = 0;
	virtual IScalarParameter & environmentVerticalOffset() = 0;
	virtual IScalarParameter & environmentHorizontalRotation() = 0;
	virtual IScalarParameter & environmentVerticalRotation() = 0;

	virtual IScalarParameter & diffuseIntensity() = 0;

	virtual IBooleanParameter & floorEnabled() = 0;
	virtual IScalarParameter & floorReflectionLevel() = 0;
	virtual IScalarParameter & floorRoughness() = 0;
	virtual IScalarParameter & floorShadowLevel() = 0;
};

class IUVMapping
{
public:
	virtual ~IUVMapping() {}

	virtual IEnumParameter & mapping() = 0;
	virtual IEnumParameter & fitTo() = 0;
	virtual IEnumParameter & uvset() = 0;
	virtual IBooleanParameter & flipU() = 0;
	virtual IBooleanParameter & flipV() = 0;
	virtual IBooleanParameter & normalize() = 0;
	virtual IVec3Parameter & scale() = 0;
	virtual IVec2Parameter & repeat() = 0;

	virtual void accept() = 0;
};

class IEditNormals
{
public:
	virtual ~IEditNormals() {}

	virtual IBooleanParameter & calculateNormals() = 0;
	virtual IEnumParameter & makeEdges() = 0;
	virtual IScalarParameter & maxSoftAngle() = 0;
	virtual IBooleanParameter & flipNormals() = 0;
	virtual IBooleanParameter & flipFacing() = 0;

	virtual void accept() = 0;
};

class IPivotParameters
{
public:
	virtual ~IPivotParameters() {}

	virtual IEnumParameter & calculatePivot() = 0;
	virtual IBooleanParameter & includingChildrenNodes() = 0;
	virtual IEnumParameter & pivotX() = 0;
	virtual IEnumParameter & pivotY() = 0;
	virtual IEnumParameter & pivotZ() = 0;

	virtual void accept() = 0;
};

class ISnapshotInterpolator
{
public:
	virtual ~ISnapshotInterpolator() {}

	virtual IIntegerParameter & transitionFrame() = 0;
	virtual IIntegerParameter & transitionFrameCount() = 0;
	virtual IScalarParameter & transitionExponent() = 0;
	virtual IScalarParameter & motionBlur() = 0;
};

typedef std::set<ISceneElement *>   Selection;
typedef std::vector<IGeometry *>    GeomSelection;
typedef std::vector<INode *>        NodeSelection;

class SHAREDLIB_EXPORT IScene : public QObject
{
	Q_OBJECT

protected:
	virtual ~IScene() {}

public:
	virtual INode & root() = 0;

	virtual const QJsonObject & metadata() const = 0;
	virtual void setMetadata(QJsonObject metadata) = 0;

	virtual ISceneProperties & properties() = 0;
	virtual class IMaterialManager & materialManager() = 0;
	virtual class ISnapshotManager & snapshotManager() = 0;
	virtual class IRenderLayerManager & renderLayerManager() = 0;

	virtual class ICamera & camera() = 0;

	virtual const Selection & selection() const = 0;
	virtual NodeSelection nodeSelection() const = 0;
	virtual GeomSelection geomSelection() const = 0;
	virtual bool isSelected(const ISceneElement *) const = 0;
	virtual void setSelection(const Selection & selection, eSelectionOperation op) = 0;

	virtual void deleteElements(const Selection & elements) = 0;
	virtual void setVisible(const NodeSelection & nodes, bool visible) = 0;

	virtual void moveNodes(const NodeSelection &nodes, INode * target, size_t pos, const QString & groupName) = 0;
	virtual INode *moveMeshes(const GeomSelection & geoms, bool deleteEmptyNodes, INode * targetNode) = 0;
	virtual IGeometry *mergeMeshes(const GeomSelection & geoms) = 0;

	virtual void setMaterial(const GeomSelection & geoms, IMaterial * material) = 0;
	virtual void replaceMaterial(IMaterial * oldMaterial, IMaterial * newMaterial) = 0;

	virtual void collectFileNames(QSet<QString> & res) const = 0;
	virtual void updateFileNames(const QMap<QString, QString> &) = 0;

	virtual class ILightNode * addLight() = 0;
	virtual class IDirectionalLight * addDirectionalLight() = 0;

	virtual QByteArray copyNodes(const NodeSelection & nodes) = 0;

	virtual std::unique_ptr<IUVMapping> beginUVMapping(const GeomSelection & meshes) = 0;
	virtual std::unique_ptr<IEditNormals> beginEditNormals(const GeomSelection & meshes) = 0;
	virtual std::unique_ptr<IPivotParameters> beginPivotParameters(const NodeSelection & nodes) = 0;
	virtual std::unique_ptr<ISnapshotInterpolator> beginSnapshotInterpolator(const ISnapshot & snapshot1, const ISnapshot & snapshot2) = 0;
	virtual void beginDropToSurface(const NodeSelection & nodes) = 0;

	virtual void buildCollisionScene() = 0;

	virtual class QUndoStack &undoStack() = 0;

signals:
	void selectionChanged();
	void nodeChanged(const INode * node, eNodeChanged what);
	void completelyChanged();

	void beforeSceneUpdate(const SceneModification & mod);
	void afterSceneUpdate(const SceneModification & mod);
};
