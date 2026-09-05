#pragma once

#include <QSettings>

#include "SceneTree.h"
#include "NodeInfo.h"
#include "CameraInfo.h"
#include "SceneInfo.h"
#include "MaterialInfo.h"
#include "SnapshotInfo.h"

#include "../Shared/Interfaces/Scene.h"

class MainWindow;
class LibraryCollection;
class ICoreInstance;
class ICoreRenderer;
class RenderingParametersProxy;
class QActionGroup;

#include "RenderingProxy.h"

#include <QQuickFramebufferObject>

class FbItem : public QQuickFramebufferObject
{
	Q_OBJECT
	Q_PROPERTY(QString uuid MEMBER m_uuid CONSTANT)

	const QString m_uuid;
	ICoreRenderer * m_renderer = nullptr;

	QQuickFramebufferObject::Renderer *createRenderer() const override;

	void hoverMoveEvent(QHoverEvent *event) override;
	void mousePressEvent(QMouseEvent * event) override;
	void mouseMoveEvent(QMouseEvent * event) override;
	void mouseReleaseEvent(QMouseEvent * event) override;
	void wheelEvent(QWheelEvent * event) override;

public:
	FbItem();
	~FbItem() = default;
};

class MainWidget : public QQuickWidget
{
	Q_OBJECT
	Q_PROPERTY(int gizmo READ getGizmo WRITE setGizmo NOTIFY gizmoChanged)
	Q_PROPERTY(bool hasSelection MEMBER m_hasSelection NOTIFY selectionChanged)
	Q_PROPERTY(bool hasNodeSelection MEMBER m_hasNodeSelection NOTIFY selectionChanged)
	Q_PROPERTY(bool hasGeomSelection MEMBER m_hasGeomSelection NOTIFY selectionChanged)
	Q_PROPERTY(bool hasCommonNodeSelection MEMBER m_hasCommonNodeSelection NOTIFY selectionChanged)
	Q_PROPERTY(bool hasCopiedNodes MEMBER m_hasCopiedNodes NOTIFY clipboardChanged)
	Q_PROPERTY(bool canCombineMeshes MEMBER m_canCombineMeshes NOTIFY selectionChanged)
	Q_PROPERTY(bool canGroupSelected MEMBER m_canGroupSelected NOTIFY selectionChanged)
	Q_PROPERTY(bool undoEnabled READ getUndoEnabled NOTIFY undoChanged)
	Q_PROPERTY(bool redoEnabled READ getRedoEnabled NOTIFY undoChanged)
	Q_PROPERTY(bool canOpenMetadataEditor READ canOpenMetadataEditor NOTIFY selectionChanged)

	MainWindow & m_mainWindow;
	IApplicationContext & m_appCtx;
	QSettings & m_settings;

	QScopedPointer<LibraryCollection> m_materialLibrary;
	QScopedPointer<LibraryCollection> m_environmentLibrary;
	QScopedPointer<LibraryCollection> m_textureLibrary;
	QScopedPointer<LibraryCollection> m_shapeLibrary;
	QScopedPointer<class QMenu> m_menu;

	QAction * m_fileImport = nullptr;
	QAction * m_fileSave = nullptr;
	QAction * m_fileSaveAs = nullptr;
	QAction * m_fileClose = nullptr;
	QAction * m_editUndo = nullptr;
	QAction * m_editRedo = nullptr;
	QAction * m_editCut = nullptr;
	QAction * m_editCopy = nullptr;
	QAction * m_editPaste = nullptr;
	QAction * m_editDelete = nullptr;
	QAction * m_editHideShow = nullptr;
	QAction * m_sceneExtractMeshes = nullptr;
	QAction * m_sceneCombineMeshes = nullptr;
	QAction * m_sceneGroupSelected = nullptr;

	QAction * m_sceneAddLight = nullptr;
	QAction * m_sceneAddDirectionalLight = nullptr;
	QActionGroup * m_sceneGizmo = nullptr;

	QAction * m_toolsRender = nullptr;
	QAction * m_toolsFitToView = nullptr;
	QAction * m_toolsDropToSurface = nullptr;
	QAction * m_toolsEditNormals = nullptr;
	QAction * m_toolsUVMapping = nullptr;
	QAction * m_toolsPivotParameters = nullptr;
	QAction * m_toolsReloadImages = nullptr;
	QAction * m_toolsResourceManager = nullptr;
	QAction * m_toolsRenderingLayersManager = nullptr;

	QMenu * m_viewDebug = nullptr;
	QAction * m_showNormal = nullptr;
	QScopedPointer<QActionGroup> m_viewLeftTabs;
	QScopedPointer<QActionGroup> m_viewRightTabs;

	QMenu * m_navigate = nullptr;
	QAction * m_previousTab = nullptr;
	QAction * m_nextTab = nullptr;

	QMenu * m_help = nullptr;
	QAction * m_helpOpenLogFolder = nullptr;
	QAction * m_helpOpenLibraryFolder = nullptr;

public:
	struct TabContext {
		QSharedPointer<ICoreInstance> core;
		ICoreRenderer * renderer = nullptr;
		QString title;
		QString filePath;
		QString uuid;
		std::unique_ptr<SceneTree> sceneTree;
		std::unique_ptr<SceneInfo> sceneInfo;
		std::unique_ptr<CameraInfo> cameraInfo;
		std::unique_ptr<MaterialInfo> materialInfo;
		std::unique_ptr<SnapshotInfo> snapshotInfo;
		std::unique_ptr<NodeInfo> nodeInfo;
	};

private:
	std::vector<std::unique_ptr<TabContext>> m_tabs;
	TabContext * m_ctx = nullptr;
	ICoreInstance * m_core = nullptr;
	IScene * m_scene = nullptr;

	std::unique_ptr<class FitToViewProxy> m_fitToView;
	std::unique_ptr<class UVMappingProxy> m_uvMapping;
	std::unique_ptr<class EditNormalsProxy> m_editNormals;
	std::unique_ptr<class PivotParametersProxy> m_pivotParameters;
	std::unique_ptr<class AppSettingsProxy> m_appSettings;
	std::unique_ptr<class RenderingParametersProxy> m_renderingParameters;
	std::unique_ptr<class ResourceManagerProxy> m_resourceManager;
	std::unique_ptr<class RenderingLayersManagerProxy> m_renderingLayersManager;
	std::unique_ptr<class RenderingProxy> m_rendering;
	std::unique_ptr<class PreviewRenderer> m_previewRenderer;
	ColorImageProvider * m_imageProvider;
	RenderingContext m_activeRenderOptions;

	bool m_hasSelection = false;
	bool m_hasNodeSelection = false;
	bool m_hasGeomSelection = false;
	bool m_hasCommonNodeSelection = false;
	bool m_hasCopiedNodes = false;
	bool m_canCombineMeshes = false;
	bool m_canGroupSelected = false;
	bool m_clearUndoAfterSave = false;
	QString m_savingFilePath;
	QString m_savingTmlFilePath;

	enum eDragType {
		DragType_None,
		DragType_Material,
		DragType_Image,
		DragType_ImageHDR,
		DragType_Model,
	} m_dragType;

	void setupMenu();
	void activateLeftTab(int);
	void activateRightTab(int);
	void onTabChanged(int index);
	void fetchSettings();
	int getTabIndex(QString uuid) const;
	QString getLastScenePath() const;
	void addRecentFile(const QString & path, bool checkExistence = false);

	void updateViewportGeometry();

public:
	MainWidget(MainWindow & mainWindow, IApplicationContext & ctx);
	virtual ~MainWidget();

	QImage currentMaterialPreview() const;

	bool exit();

	bool openFile(const QString & path, bool async);
	bool saveFile(bool saveAs);
	void openRecent();

	int getGizmo() const;
	void setGizmo(int mode);
	bool getUndoEnabled() const;
	bool getRedoEnabled() const;
	bool canOpenMetadataEditor() const;

	Q_INVOKABLE void onLeftTabActivated(int);
	Q_INVOKABLE void onRightTabActivated(int);
	Q_INVOKABLE void activateTab(QString uuid);
	Q_INVOKABLE void closeCurrentTab();

	Q_INVOKABLE void editCut();
	Q_INVOKABLE void editCopy();
	Q_INVOKABLE void editPaste();
	Q_INVOKABLE void editDelete();
	Q_INVOKABLE void editShowHide();
	Q_INVOKABLE void undo();
	Q_INVOKABLE void redo();
	Q_INVOKABLE void extractMeshes();
	Q_INVOKABLE void combineMeshes();
	Q_INVOKABLE void groupSelected();

	Q_INVOKABLE bool onDragEntered(float x, float y, const QString & url, bool tabBarArea);
	Q_INVOKABLE bool onDragPositionChanged(float x, float y, const QString & url, bool tabBarArea);
	Q_INVOKABLE bool onDropped(float x, float y, const QString & url, bool tabBarArea);
	Q_INVOKABLE bool isImageURL(const QString & url) const;
	Q_INVOKABLE QString urlToLocalFile(const QVariant & url) const;

	Q_INVOKABLE void beginRenderingParameters();
	Q_INVOKABLE void acceptRenderingParameters();
	Q_INVOKABLE void endRenderingParameters();

	Q_INVOKABLE void beginRendering();
	Q_INVOKABLE void endRendering();

	Q_INVOKABLE void beginFitToView();
	Q_INVOKABLE void acceptFitToView();
	Q_INVOKABLE void endFitToView();

	Q_INVOKABLE void dropToSurface();
	Q_INVOKABLE void putOnTheFloor() { dropToSurface(); }

	Q_INVOKABLE void beginEditNormals();
	Q_INVOKABLE void acceptEditNormals();
	Q_INVOKABLE void endEditNormals();

	Q_INVOKABLE void beginUVMapping();
	Q_INVOKABLE void acceptUVMapping();
	Q_INVOKABLE void endUVMapping();

	Q_INVOKABLE void beginPivotParameters();
	Q_INVOKABLE void acceptPivotParameters();
	Q_INVOKABLE void endPivotParameters();
	Q_INVOKABLE void beginNodeMetadata();
	Q_INVOKABLE void beginResourceManager();
	Q_INVOKABLE void endResourceManager();
	Q_INVOKABLE void beginRenderingLayersManager();
	Q_INVOKABLE void endRenderingLayersManager();
	Q_INVOKABLE void beginAppSettings();
	Q_INVOKABLE void acceptAppSettings();
	Q_INVOKABLE void endAppSettings();
	Q_INVOKABLE void beginAbout();
	Q_INVOKABLE void beginTextureDialog();
	Q_INVOKABLE void endTextureDialog();

public slots:
	bool close();

	void onNodeChanged(const INode *, eNodeChanged);
	void onSelectionChanged();
	void onUndoChanged();
	void onClipboardChanged();

	void onProgressStarted(const QString & operation);
	void onProgressUpdated(float progress);
	void onProgressFinished();
	void onSceneLoaded();
	void onSceneSaved();
	void onSceneLoadingFailed(const QString & error);
	void onSceneSavingFailed(const QString & error);

signals:
	void gizmoChanged();
	void undoChanged();
	void selectionChanged();
	void clipboardChanged();
};
