#include "MainWidget.h"
#include "MainWindow.h"
#include "CollectionView.h"
#include "FitToViewProxy.h"
#include "EditNormalsProxy.h"
#include "UVMappingProxy.h"
#include "PivotParametersProxy.h"
#include "RenderingProxy.h"
#include "RenderingParametersProxy.h"
#include "AppSettingsProxy.h"
#include "ResourceManagerProxy.h"
#include "RenderingLayersManagerProxy.h"
#include "PreviewRenderer.h"
#include <QQuickImageProvider>

class MaterialImageProvider : public QQuickImageProvider {
	MainWidget * m_widget;
public:
	MaterialImageProvider(MainWidget * widget)
		: QQuickImageProvider(QQuickImageProvider::Image)
		, m_widget(widget) {}

	QImage requestImage(const QString & id, QSize * size, const QSize & requestedSize) override {
		QImage img = m_widget->currentMaterialPreview();
		if (size) {
			*size = img.size();
		}
		return img;
	}
};

#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QUuid>
#include <QMenuBar>
#include <QActionGroup>
#include <QClipboard>
#include <QApplication>
#include <QMimeData>
#include <QDebug>
#include <QMessageBox>
#include <QUndoStack>
#include <QTimer>
#include <QItemSelectionModel>
#include <QDesktopServices>
#include <QUrl>

#include "../Shared/Interfaces/CoreInstance.h"
#include "../Shared/Interfaces/Node.h"
#include "../Shared/Interfaces/CoreRenderer.h"
#include "../Shared/Interfaces/Geometry.h"
#include "../Shared/Interfaces/Material.h"
#include "../Shared/Interfaces/MaterialManager.h"
#include "../Shared/Interfaces/SerializationContext.h"
#include "../Shared/Interfaces/SnapshotManager.h"
#include "../Shared/Interfaces/ModelLoader.h"
#include "../Shared/Interfaces/ModelSaver.h"
#include "../Shared/Utils/FileUtils.h"

static QString s_newUuid;
static ICoreRenderer * s_newRenderer = nullptr;
static const QString SETTINGS_RECENT_FILES = "recent";
static const QString SETTINGS_SHOW_NORMAL = "showNormal";
static const QString LOCAL_MATERIAL_PREFIX("miray://material/");
static const char* CLIPBOARD_MIME_TYPE = "org.miray/copied-nodes";
constexpr int MAX_RECENT_FILES_COUNT = 50;

// ------------------------------------------------------------------------ //

#ifdef __APPLE__

class MenuLocker
{
	QMenuBar * const m_menuBar;
public:
	MenuLocker(QMainWindow * mainWindow) : m_menuBar(mainWindow->menuBar()) { m_menuBar->setEnabled(false); }
	~MenuLocker() { m_menuBar->setEnabled(true); }
};

#define MENU_LOCKER		MenuLocker menuLocker(m_mainWindow);
#define LOCK_MENU		m_mainWindow.menuBar()->setEnabled(false);
#define UNLOCK_MENU		m_mainWindow.menuBar()->setEnabled(true);

#else

#define MENU_LOCKER
#define LOCK_MENU
#define UNLOCK_MENU

#endif

// ------------------------------------------------------------------------ //

FbItem::FbItem() : m_uuid(s_newUuid), m_renderer(s_newRenderer)
{
	setMirrorVertically(true);
	setAcceptHoverEvents(true);
	setAcceptTouchEvents(true);
	setAcceptedMouseButtons(Qt::AllButtons);
}

QQuickFramebufferObject::Renderer *FbItem::createRenderer() const { return m_renderer; }

void FbItem::hoverMoveEvent(QHoverEvent *event) { m_renderer->hoverMoveEvent(event); }
void FbItem::mousePressEvent(QMouseEvent * event) { forceActiveFocus(); m_renderer->mousePressEvent(event); event->accept(); }
void FbItem::mouseMoveEvent(QMouseEvent * event) { m_renderer->mouseMoveEvent(event); event->accept(); }
void FbItem::mouseReleaseEvent(QMouseEvent * event) { m_renderer->mouseReleaseEvent(event); event->accept(); }
void FbItem::wheelEvent(QWheelEvent * event) { m_renderer->wheelEvent(event); event->accept(); }

static QIcon createIcon(const QString & name)
{
	QIcon icon;
	icon.addFile(QString(":/toolbar/%1").arg(name));
	return icon;
}

static QIcon createToggleIcon(const QString & name)
{
	const auto normal = QString(":/toolbar/gizmo-%1-normal.png").arg(name);
	// const auto hover = QString(":/toolbar/gizmo-%1-hover.svg").arg(name);
	const auto pressed = QString(":/toolbar/gizmo-%1-pressed.svg").arg(name);
	const auto disabled = QString(":/toolbar/gizmo-%1-disabled.svg").arg(name);
	QIcon icon;
	icon.addFile(normal, QSize(), QIcon::Normal, QIcon::Off);
	icon.addFile(disabled, QSize(), QIcon::Disabled, QIcon::Off);
	icon.addFile(pressed, QSize(), QIcon::Normal, QIcon::On);
	return icon;
}

MainWidget::MainWidget(MainWindow & mainWindow, IApplicationContext & ctx)
	: QQuickWidget(&mainWindow)
	, m_mainWindow(mainWindow)
	, m_appCtx(ctx)
	, m_materialLibrary(new LibraryCollection(ctx.getFullResourcePath("miray://Library/Materials"), { "*.mirayMaterial" }))
	, m_environmentLibrary(new LibraryCollection(ctx.getFullResourcePath("miray://Library/Environments"), { "*.exr", "*.hdr" }))
	, m_textureLibrary(new LibraryCollection(ctx.getFullResourcePath("miray://Library/Textures"), { "*.jpg", "*.png", "*.exr" }))
	, m_shapeLibrary(new LibraryCollection(ctx.getFullResourcePath("miray://Library/Shapes"), { "*.mirayScene" }))
	, m_settings(mainWindow.settings())
{
	qmlRegisterType<FbItem>("FbItem", 1, 0, "FbItem");

	m_imageProvider = new ColorImageProvider();
	engine()->addImageProvider("Preview", m_imageProvider);
	engine()->addImageProvider("materialpreview", new MaterialImageProvider(this));

	m_previewRenderer = std::make_unique<PreviewRenderer>(ctx);

	auto context = rootContext();
	context->setContextProperty("mainWindow", quickWindow());
	context->setContextProperty("presenter", this);

	context->setContextProperty("materialsDataModel", m_materialLibrary->filterModel());
	context->setContextProperty("materialsTreeModel", m_materialLibrary->treeModel());
	context->setContextProperty("environmentsDataModel", m_environmentLibrary->filterModel());
	context->setContextProperty("environmentsTreeModel", m_environmentLibrary->treeModel());
	context->setContextProperty("texturesDataModel", m_textureLibrary->filterModel());
	context->setContextProperty("texturesTreeModel", m_textureLibrary->treeModel());
	context->setContextProperty("shapesDataModel", m_shapeLibrary->filterModel());
	context->setContextProperty("shapesTreeModel", m_shapeLibrary->treeModel());

	context->setContextProperty("appSettingsModel", nullptr);
	context->setContextProperty("sceneModel", nullptr);

	context->setContextProperty("sceneTreePresenter", nullptr);
	context->setContextProperty("sceneTreeModel", nullptr);

	context->setContextProperty("nodeModel", nullptr);

	context->setContextProperty("materialsList", nullptr);
	context->setContextProperty("material", nullptr);
	context->setContextProperty("materialPresenter", nullptr);

	context->setContextProperty("cameraModel", nullptr);

	context->setContextProperty("snapshotsList", nullptr);
	context->setContextProperty("snapshotPresenter", nullptr);

	context->setContextProperty("fitToViewModel", nullptr);
	context->setContextProperty("editNormalsModel", nullptr);
	context->setContextProperty("uvMappingModel", nullptr);
	context->setContextProperty("pivotParamsModel", nullptr);
	context->setContextProperty("renderingParametersModel", nullptr);
	context->setContextProperty("renderingModel", nullptr);
	context->setContextProperty("resManagerPresenter", nullptr);
	context->setContextProperty("renderingLayersManagerPresenter", nullptr);

	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	setResizeMode(QQuickWidget::SizeRootObjectToView);
	setSource(QUrl("qrc:/qml/main.qml"));

	setupMenu();

	onTabChanged(-1);

	connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(onClipboardChanged()));
}

MainWidget::~MainWidget()
{
}

bool MainWidget::exit()
{
	if (!m_mainWindow.menuBar()->isEnabled())
		return false;

	while (!m_tabs.empty()) {
		if (!close())
			return false;
	}

	return true;
}

void MainWidget::setupMenu()
{
	auto menuBar = m_mainWindow.menuBar();
	auto file = menuBar->addMenu("File");
	auto edit = menuBar->addMenu("Edit");
	auto scene = menuBar->addMenu("Scene");
	auto tools = menuBar->addMenu("Tools");
	auto view = menuBar->addMenu("View");
	m_navigate = menuBar->addMenu("Navigate");

	file->addAction("New", this, [this]() {
		openFile(QString(), false);
	}, QKeySequence(QKeySequence::New));
	m_fileImport = file->addAction("Import...", this, [this]() {
		auto fileName = getOpenFileName(this, "Import...", "", getLastScenePath(), m_appCtx.modelLoadersRegistry()->loadingFilters());
		if (!fileName.isEmpty())
			openFile(fileName, true);
	}, QKeySequence(Qt::CTRL + Qt::Key_I));

	file->addSeparator();

	file->addAction("Open...", this, [this]() {
		auto fileName = getOpenFileName(this, "Open...", "mirayScene", getLastScenePath(), m_appCtx.modelLoadersRegistry()->loadingFilters());
		if (!fileName.isEmpty())
			openFile(fileName, true);
	}, QKeySequence(QKeySequence::Open));
	auto recentMenu = file->addMenu("Open Recent");
	connect(recentMenu, &QMenu::aboutToShow, this, &MainWidget::openRecent);

	file->addSeparator();

	m_fileSave = file->addAction("Save", this, [this]() {
		saveFile(false);
	}, QKeySequence(QKeySequence::Save));
	m_fileSaveAs = file->addAction("Save As...", this, [this]() {
		saveFile(true);
	}, QKeySequence(QKeySequence::SaveAs));

	file->addSeparator();

	m_fileClose = file->addAction("Close", this, SLOT(close()), QKeySequence(QKeySequence::Close));

	m_editUndo = edit->addAction("Undo", this, &MainWidget::undo, QKeySequence(QKeySequence::Undo));
	m_editRedo = edit->addAction("Redo", this, &MainWidget::redo, QKeySequence(QKeySequence::Redo));
	m_editUndo->setIcon(createToggleIcon("undo"));
	m_editRedo->setIcon(createToggleIcon("redo"));

	edit->addSeparator();

	m_editCut = edit->addAction("Cut", this, &MainWidget::editCut, QKeySequence(QKeySequence::Cut));
	m_editCopy = edit->addAction("Copy", this, &MainWidget::editCopy, QKeySequence(QKeySequence::Copy));
	m_editPaste = edit->addAction("Paste", this, &MainWidget::editPaste, QKeySequence(QKeySequence::Paste));
	m_editDelete = edit->addAction("Delete", this, &MainWidget::editDelete, QKeySequence(QKeySequence::Delete));

	edit->addSeparator();

	m_editHideShow = edit->addAction("Hide Selection", this, [this]() {
		if (!m_scene) return;
		auto selection = m_scene->nodeSelection();
		bool visible = false;
		for (auto node : selection) {
			if (node->visible().get()) {
				visible = true;
				break;
			}
		}
		m_scene->setVisible(selection, !visible);
	}, QKeySequence(Qt::Key_H));

	m_sceneAddLight = scene->addAction("Add Light", this, [this]() {
		if (m_scene) m_scene->addLight();
	});
	m_sceneAddDirectionalLight = scene->addAction("Add Directional Light", this, [this]() {
		if (m_scene) m_scene->addDirectionalLight();
	});

	scene->addSeparator();

	m_sceneGizmo = new QActionGroup(this);
	m_sceneGizmo->addAction(scene->addAction("Selection", this, [this]() { setGizmo(GIZMO_NONE); }, QKeySequence(Qt::CTRL + Qt::Key_1)));
	m_sceneGizmo->addAction(scene->addAction("Translation", this, [this]() { setGizmo(GIZMO_MOVE); }, QKeySequence(Qt::CTRL + Qt::Key_2)));
	m_sceneGizmo->addAction(scene->addAction("Rotation", this, [this]() { setGizmo(GIZMO_ROTATE); }, QKeySequence(Qt::CTRL + Qt::Key_3)));
	m_sceneGizmo->addAction(scene->addAction("Scaling", this, [this]() { setGizmo(GIZMO_SCALE); }, QKeySequence(Qt::CTRL + Qt::Key_4)));
	m_sceneGizmo->addAction(scene->addAction("Move Camera Target", this, [this]() { setGizmo(GIZMO_CAMERA); }, QKeySequence(Qt::CTRL + Qt::Key_5)));

	for (auto * action : m_sceneGizmo->actions())
		action->setCheckable(true);
	m_sceneGizmo->actions()[0]->setChecked(true);

	scene->addSeparator();

	m_sceneExtractMeshes = scene->addAction("Extract Meshes", this, SLOT(extractMeshes()));
	m_sceneCombineMeshes = scene->addAction("Combine Meshes", this, SLOT(combineMeshes()));
	m_sceneGroupSelected = scene->addAction("Group Selected", this, SLOT(groupSelected()));

	// ---------------- Tools menu ---------------- //

	m_toolsRender = tools->addAction("Render Scene...", this, [this]() {
		QVariant retVal;
		QMetaObject::invokeMethod(rootObject(), "runRenderingParameters", Q_RETURN_ARG(QVariant, retVal));
	}, QKeySequence(Qt::CTRL + Qt::Key_R));
	m_toolsRender->setIcon(createIcon("render-frame.png"));

	tools->addSeparator();

	m_toolsFitToView = tools->addAction("Fit To View...", this, [this]() {
		QVariant retVal;
		QMetaObject::invokeMethod(rootObject(), "runFitToView", Q_RETURN_ARG(QVariant, retVal));
	});

	m_toolsDropToSurface = tools->addAction("Drop To Surface", this, [this]() {
		if (m_scene)
			m_scene->beginDropToSurface(m_scene->nodeSelection());
	});

	m_toolsUVMapping = tools->addAction("Generate UV Mapping...", this, [this]() {
		QVariant retVal;
		QMetaObject::invokeMethod(rootObject(), "runUVMapping", Q_RETURN_ARG(QVariant, retVal));
	});

	m_toolsEditNormals = tools->addAction("Edit Normals...", this, [this]() {
		QVariant retVal;
		QMetaObject::invokeMethod(rootObject(), "runEditNormals", Q_RETURN_ARG(QVariant, retVal));
	});

	m_toolsPivotParameters = tools->addAction("Edit Pivot Points...", this, [this]() {
		QVariant retVal;
		QMetaObject::invokeMethod(rootObject(), "runPivotParameters", Q_RETURN_ARG(QVariant, retVal));
	});

	m_toolsReloadImages = tools->addAction("Reload Images", this, [this]() {
		m_appCtx.imageManager()->reloadImages();
	});

	tools->addSeparator();

	m_toolsResourceManager = tools->addAction("Resource Manager...", this, [this]() {
		QVariant retVal;
		QMetaObject::invokeMethod(rootObject(), "runResourceManager", Q_RETURN_ARG(QVariant, retVal));
	});

	m_toolsRenderingLayersManager = tools->addAction("Rendering Layers Manager...", this, [this]() {
		QVariant retVal;
		QMetaObject::invokeMethod(rootObject(), "runRenderingLayersManager", Q_RETURN_ARG(QVariant, retVal));
	});

	tools->addSeparator();

	tools->addAction("Settings...", this, [this]() {
		QVariant retVal;
		QMetaObject::invokeMethod(rootObject(), "runAppSettings", Q_RETURN_ARG(QVariant, retVal));
	}, QKeySequence(QKeySequence::Preferences));

	// ---------------- View menu ---------------- //

	m_viewDebug = view->addMenu("Debug");

	m_showNormal = m_viewDebug->addAction("Show Normal", this, [this]() {
		m_settings.setValue(SETTINGS_SHOW_NORMAL, m_showNormal->isChecked());
		if (m_ctx)
			m_ctx->renderer->setShowNormal(m_showNormal->isChecked());
	});
	m_showNormal->setCheckable(true);
	m_showNormal->setChecked(m_settings.value(SETTINGS_SHOW_NORMAL, false).toBool());

	view->addSeparator();
	m_viewLeftTabs.reset(new QActionGroup(this));
	m_viewLeftTabs->addAction(view->addAction("Library Materials", this, [this]() { activateLeftTab(0); }));
	m_viewLeftTabs->addAction(view->addAction("Library Environment Maps", this, [this]() { activateLeftTab(1); }));
	m_viewLeftTabs->addAction(view->addAction("Library Textures", this, [this]() { activateLeftTab(2); }));
	m_viewLeftTabs->addAction(view->addAction("Library Shapes", this, [this]() { activateLeftTab(3); }));
	m_viewLeftTabs->addAction(view->addAction("Scene Tree", this, [this]() { activateLeftTab(4); }));

	view->addSeparator();
	m_viewRightTabs.reset(new QActionGroup(this));
	m_viewRightTabs->addAction(view->addAction("Node Parameters", this, [this]() { activateRightTab(0); }));
	m_viewRightTabs->addAction(view->addAction("Scene Materials", this, [this]() { activateRightTab(1); }));
	m_viewRightTabs->addAction(view->addAction("Camera", this, [this]() { activateRightTab(2); }));
	m_viewRightTabs->addAction(view->addAction("Scene Settings", this, [this]() { activateRightTab(3); }));
	m_viewRightTabs->addAction(view->addAction("Snapshots", this, [this]() { activateRightTab(4); }));
	for (auto * action : m_viewLeftTabs->actions())
		action->setCheckable(true);
	for (auto * action : m_viewRightTabs->actions())
		action->setCheckable(true);
	m_viewLeftTabs->actions()[3]->setChecked(true); // set "Library Shapes" checked by default
	m_viewRightTabs->actions()[0]->setChecked(true);

	m_previousTab = m_navigate->addAction("Show Previous Tab", this, [this]() {
		QVariant retVal;
		QMetaObject::invokeMethod(rootObject(), "previousTab", Q_RETURN_ARG(QVariant, retVal));
	}, QKeySequence::PreviousChild);
	m_previousTab->setEnabled(false);

	m_nextTab = m_navigate->addAction("Show Next Tab", this, [this]() {
		QVariant retVal;
		QMetaObject::invokeMethod(rootObject(), "nextTab", Q_RETURN_ARG(QVariant, retVal));
	}, QKeySequence::NextChild);
	m_nextTab->setEnabled(false);

	m_navigate->addSeparator();

	// ---------------- Help menu ---------------- //

	m_help = menuBar->addMenu("Help");

	m_helpOpenLogFolder = m_help->addAction("Open log folder...", this, []() {
		const auto dir = standardTempLocation() + "/miray";
		QDir().mkpath(dir);
		QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
	});

	m_helpOpenLibraryFolder = m_help->addAction("Open library folder...", this, [this]() {
		const auto dir = m_appCtx.getFullResourcePath("miray://Library");
		QDir().mkpath(dir);
		QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
	});
}

void MainWidget::activateLeftTab(int index)
{
	QVariant retVal;
	QMetaObject::invokeMethod(rootObject(), "activateLeftTab", Q_RETURN_ARG(QVariant, retVal), Q_ARG(QVariant, index));
}

void MainWidget::activateRightTab(int index)
{
	QVariant retVal;
	QMetaObject::invokeMethod(rootObject(), "activateRightTab", Q_RETURN_ARG(QVariant, retVal), Q_ARG(QVariant, index));
}

void MainWidget::onLeftTabActivated(int index)
{
	if (index >= 0 && index < m_viewLeftTabs->actions().size())
		m_viewLeftTabs->actions()[index]->setChecked(true);
}

void MainWidget::onRightTabActivated(int index)
{
	if (index >= 0 && index < m_viewRightTabs->actions().size())
		m_viewRightTabs->actions()[index]->setChecked(true);
}

bool MainWidget::openFile(const QString & path, bool async)
{
	qDebug() << "openFile" << path << async;
	QSharedPointer<ICoreInstance> core(m_mainWindow.appContext().createCoreInstance());
	if (!core)
		return false;

	const auto isInternalResource = m_appCtx.isInternalResource(path);
	if (!path.isEmpty()) {
		try {
			connect(core.data(), SIGNAL(progressStarted(const QString &)), this, SLOT(onProgressStarted(const QString &)));
			connect(core.data(), SIGNAL(progressUpdated(float)), this, SLOT(onProgressUpdated(float)));
			connect(core.data(), SIGNAL(progressFinished()), this, SLOT(onProgressFinished()));
			connect(core.data(), SIGNAL(sceneLoaded()), this, SLOT(onSceneLoaded()));
			connect(core.data(), SIGNAL(sceneSaved()), this, SLOT(onSceneSaved()));
			connect(core.data(), SIGNAL(sceneLoadingFailed(const QString &)), this, SLOT(onSceneLoadingFailed(const QString &)));
			connect(core.data(), SIGNAL(sceneSavingFailed(const QString &)), this, SLOT(onSceneSavingFailed(const QString &)));

			if (!isInternalResource)
				addRecentFile(path);

			core->loadFile(path, LoadingParameters::forSceneLoading().async(async));
		} catch (...) {
			return false;
		}
	}

	s_newRenderer = core->createRenderer(devicePixelRatioF(), false);
	s_newUuid = QUuid::createUuid().toString();
	const auto untitled = path.isEmpty() || isInternalResource;
	const auto title = untitled ? "Untitled" : QFileInfo(path).completeBaseName();
	auto scene = core->getScene();

	m_tabs.emplace_back(std::make_unique<TabContext>(core, s_newRenderer, title, isInternalResource ? "" : path, s_newUuid,
		std::make_unique<SceneTree>(scene, this),
		std::make_unique<SceneInfo>(scene->properties()),
		std::make_unique<CameraInfo>(scene->camera()),
		std::make_unique<MaterialInfo>(*scene, m_previewRenderer.get()),
		std::make_unique<SnapshotInfo>(scene->snapshotManager())
	));
	m_previousTab->setEnabled(m_tabs.size() > 1);
	m_nextTab->setEnabled(m_tabs.size() > 1);

	QMetaObject::invokeMethod(rootObject(), "createTab", Q_ARG(QVariant, title));

	return true;
}

bool MainWidget::saveFile(bool saveAs)
{
	if (!m_ctx || !m_core)
		return false;

	QString savingPath = m_ctx->filePath;
	if (savingPath.isEmpty()) {
		savingPath = getLastScenePath() + "/" + m_ctx->title + ".mirayScene";
		saveAs = true;
	}

	if (saveAs) {
		savingPath = getSaveFileName(&m_mainWindow, "Save Scene", "mirayScene", savingPath, m_appCtx.modelSaversRegistry()->savingFilters());
		if (savingPath.isEmpty())
			return false;
	}

	try {
		addRecentFile(savingPath);
		m_savingFilePath = savingPath;
		m_savingTmlFilePath = savingPath;
		m_savingTmlFilePath.insert(savingPath.lastIndexOf("."), "~tmp");
		m_clearUndoAfterSave = true;
		m_core->saveFile(m_savingTmlFilePath, SavingParameters::forSceneSaving());
		m_ctx->filePath = savingPath;
		m_ctx->title = QFileInfo(savingPath).completeBaseName();
		QMetaObject::invokeMethod(rootObject(), "setTabTitle", Q_ARG(QVariant, m_ctx->title));
	} catch (...) {
		return false;
	}

	return true;
}

QString MainWidget::getLastScenePath() const
{
	auto path = m_settings.value(SETTINGS_RECENT_FILES, QStringList()).toStringList().value(0);
	return path.isEmpty() ? standardDesktopLocation() : QFileInfo(path).absolutePath();
}

void MainWidget::addRecentFile(const QString & path, bool checkExistence)
{
	if (m_appCtx.isInternalResource(path) || (checkExistence && !QFileInfo(path).exists()))
		return;

	auto files = m_settings.value(SETTINGS_RECENT_FILES, QStringList()).toStringList();
	const auto i = files.indexOf(path);
	if (i >= 0) {
		files.move(i, 0);
	} else {
		files.push_front(path);
		if (files.size() > MAX_RECENT_FILES_COUNT)
			files.pop_back();
	}
	m_settings.setValue(SETTINGS_RECENT_FILES, files);
}

void MainWidget::openRecent()
{
	auto recentMenu = qobject_cast<QMenu *>(sender());
	recentMenu->clear();

	QStringList files = m_settings.value(SETTINGS_RECENT_FILES, QStringList()).toStringList();

	for (auto &path : files) {
		auto name = path;
#ifndef Q_OS_WIN
		name.replace(QDir::homePath(), "~");
#endif
		auto *action = new QAction(name, recentMenu);
		action->setToolTip(path);
		recentMenu->addAction(action);
		QObject::connect(action, &QAction::triggered, this, [this, path]() {
			openFile(path, false);
		});
	}

	recentMenu->addSeparator();
	auto action = new QAction("Clear Menu", recentMenu);
	action->setEnabled(!files.empty());
	recentMenu->addAction(action);
	QObject::connect(action, &QAction::triggered, this, [this]() {
		m_settings.setValue(SETTINGS_RECENT_FILES, QStringList());
	});
}

bool MainWidget::close()
{
	assert(m_ctx);
	auto it = std::find_if(m_tabs.begin(), m_tabs.end(), [this](const std::unique_ptr<TabContext> & tab) { return tab.get() == m_ctx; });
	assert(it != m_tabs.end());
	if (!m_ctx || it == m_tabs.end())
		return false;

	if (m_scene && !m_scene->undoStack().isClean()) {
		const auto text = QString("Project '%1' has been changed.\nWould you like to save the changes?").arg(m_ctx->title);
		const auto reply = QMessageBox::question(this, "MiRay", text, QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
		if (reply == QMessageBox::Cancel)
			return false;

		if (reply == QMessageBox::Save && !saveFile(false))
			return false;
	}

	const auto ctx = m_ctx;
	onTabChanged(-1);
	ctx->renderer->invalidate();
	m_tabs.erase(it);
	m_previousTab->setEnabled(m_tabs.size() > 1);
	m_nextTab->setEnabled(m_tabs.size() > 1);

	QVariant retVal;
	QMetaObject::invokeMethod(rootObject(), "removeCurrentTab", Q_RETURN_ARG(QVariant, retVal));

	return true;
}

void MainWidget::onTabChanged(int index)
{
	qDebug() << "onTabChanged" << index;
	if (m_ctx) {
		m_ctx->sceneTree->setQmlContext(nullptr, nullptr);
		m_ctx->sceneInfo->setQmlContext(nullptr);
		m_ctx->materialInfo->setQmlContext(nullptr, nullptr);
		m_ctx->cameraInfo->setQmlContext(nullptr);
		m_ctx->snapshotInfo->setQmlContext(nullptr, nullptr);
		m_ctx->renderer->enablePreview(false);
	}

	if (m_scene) {
		disconnect(m_scene, SIGNAL(nodeChanged(const INode *, eNodeChanged)), this, SLOT(onNodeChanged(const INode *, eNodeChanged)));
		disconnect(m_scene, SIGNAL(selectionChanged()), this, SLOT(onSelectionChanged()));
		disconnect(&m_scene->undoStack(), SIGNAL(indexChanged(int)), this, SLOT(onUndoChanged()));
	}

	m_ctx = index >= 0 && index < m_tabs.size() ? m_tabs[index].get() : nullptr;
	m_core = m_ctx ? m_ctx->core.data() : nullptr;
	m_scene = m_core ? m_core->getScene() : nullptr;

	auto context = rootContext();
	if (m_ctx) {
		auto object = rootObject();
		m_ctx->sceneTree->setQmlContext(context, object);
		m_ctx->sceneInfo->setQmlContext(context);
		m_ctx->materialInfo->setQmlContext(context, object);
		m_ctx->cameraInfo->setQmlContext(context);
		m_ctx->snapshotInfo->setQmlContext(context, object);
		m_ctx->renderer->enablePreview(true);

		fetchSettings();
	}

	const auto enabled = m_scene != nullptr;
	m_fileImport->setEnabled(enabled);
	m_fileSave->setEnabled(enabled);
	m_fileSaveAs->setEnabled(enabled);
	m_fileClose->setEnabled(enabled);
	m_editUndo->setEnabled(enabled);
	m_editRedo->setEnabled(enabled);
	m_editCut->setEnabled(enabled);
	m_editCopy->setEnabled(enabled);
	m_editPaste->setEnabled(enabled);
	m_editDelete->setEnabled(enabled);
	m_editHideShow->setEnabled(enabled);
	m_sceneAddLight->setEnabled(enabled);
	m_sceneAddDirectionalLight->setEnabled(enabled);
	m_sceneGizmo->setEnabled(enabled);
	m_toolsRender->setEnabled(enabled);
	m_toolsFitToView->setEnabled(enabled);
	m_toolsDropToSurface->setEnabled(enabled);
	m_toolsEditNormals->setEnabled(enabled);
	m_toolsUVMapping->setEnabled(enabled);
	m_toolsPivotParameters->setEnabled(enabled);
	m_toolsReloadImages->setEnabled(enabled);
	m_toolsResourceManager->setEnabled(enabled);
	m_toolsRenderingLayersManager->setEnabled(enabled);

	if (m_scene) {
		connect(m_scene, SIGNAL(nodeChanged(const INode *, eNodeChanged)), this, SLOT(onNodeChanged(const INode *, eNodeChanged)));
		connect(m_scene, SIGNAL(selectionChanged()), this, SLOT(onSelectionChanged()));
		connect(&m_scene->undoStack(), SIGNAL(indexChanged(int)), this, SLOT(onUndoChanged()));
	}

	onSelectionChanged();
	onClipboardChanged();
	onUndoChanged();
}

int MainWidget::getTabIndex(QString uuid) const
{
	for (int i = 0; i < m_tabs.size(); i++) {
		if (m_tabs[i]->uuid == uuid)
			return i;
	}
	return -1;
}

void MainWidget::activateTab(QString uuid)
{
	int index = getTabIndex(uuid);
	if (index >= 0) {
		onTabChanged(index);
	}
}

void MainWidget::closeCurrentTab()
{
	close();
}

// ------------------------------------------------------------------------ //

bool MainWidget::onDragEntered(float x, float y, const QString & url, bool tabBarArea)
{
	qDebug() << ".onDragEntered" << x << y << url;

	auto path = url.startsWith("file://") ? QUrl(url).toLocalFile() : url;

	if (m_appCtx.modelLoadersRegistry()->canLoadFile(path, false)) {
		m_dragType = DragType_Model;
		return true;
	}

	if (tabBarArea)
		return false;

	if (m_ctx && m_scene) {
		if (path.startsWith(LOCAL_MATERIAL_PREFIX)) {
			m_dragType = DragType_Material;
			return true;
		}

		auto suffix = QFileInfo(path).suffix();

		if (!suffix.compare("mirayMaterial") || m_scene->materialManager().isIORFile(path)) {
			m_dragType = DragType_Material;
			return true;
		}

		if (!suffix.compare("hdr") || !suffix.compare("exr")) {
			m_dragType = DragType_ImageHDR;
			return true;
		}

		if (m_appCtx.imageManager()->isImageFile(path)) {
			m_dragType = DragType_Image;
			return true;
		}
	}

	m_dragType = DragType_None;
	return false;
}

bool MainWidget::onDragPositionChanged(float x, float y, const QString & url, bool tabBarArea)
{
//	qDebug() << ".onDragPositionChanged" << x << y << url;
	if (tabBarArea && m_dragType != DragType_Model)
		return  false;

	switch (m_dragType) {
		case DragType_Material:
			if (m_ctx) {
				auto geom = m_ctx->renderer->getGeometry(x, y);
				if (geom)
					return true;
			}
			return false;

		case DragType_Model:
			return true;

		case DragType_Image:
		case DragType_ImageHDR:
			return m_ctx != nullptr;

		default: break;
	}

	return false;
}

bool MainWidget::onDropped(float x, float y, const QString & url, bool tabBarArea)
{
	qDebug() << ".onDropped" << x << y << url;
	if (tabBarArea && m_dragType != DragType_Model)
		return  false;

	auto path = m_appCtx.getShortResourcePath(::urlToLocalFile(url));

	switch (m_dragType) {
		case DragType_Material:
			if (m_ctx && m_scene) {
				auto geom = m_ctx->renderer->getGeometry(x, y);
				if (geom) {
					m_scene->undoStack().beginMacro("Apply material");

					auto & materialManager = m_scene->materialManager();
					IMaterial * material = nullptr;
					if (path.startsWith(LOCAL_MATERIAL_PREFIX)) {
						path.remove(0, LOCAL_MATERIAL_PREFIX.length());
						material = materialManager.getByName(path);
					} else {
						const auto fullPath = m_appCtx.getFullResourcePath(path);
						if (materialManager.isIORFile(fullPath)) {
							material = materialManager.createFromIORFile(fullPath);
						} else {
							auto data = readFile(fullPath);
							if (!data.isEmpty())
								material = materialManager.load(data, ModelLoadingContext(), materialManager.count());
						}
					}
					if (material) {
						m_scene->setMaterial(m_scene->isSelected(geom) ? m_scene->geomSelection() : GeomSelection{ geom }, material);
						m_ctx->materialInfo->setMaterial(material); // set current material
						qDebug() << "accept material!";
					}

					m_scene->undoStack().endMacro();
					return material != nullptr;
				}
			}
			break;

		case DragType_Model:
			if (!m_ctx || tabBarArea) {
				openFile(path, false);
			} else {
				m_ctx->core->loadFile(m_appCtx.getFullResourcePath(path), LoadingParameters::forSceneImport());
				setGizmo(GIZMO_MOVE);
			}
			return true;

		case DragType_ImageHDR:
			if (m_scene) {
				m_menu.reset(new QMenu(this));

				m_menu->addAction("Environment", [this, path]() {
					m_scene->properties().environment().texture()->fileName().set(path);
				});

				m_menu->addAction("Background", [this, path]() {
					m_scene->undoStack().beginMacro("Set background");
					m_scene->properties().backgroundMode().setIndex(BackgroundMode_SphericalImage);
					m_scene->properties().background().texture()->fileName().set(path);
					m_scene->undoStack().endMacro();
				});

				m_menu->addAction("Environment + Background", [this, path]() {
					m_scene->undoStack().beginMacro("Set envrironment + background");
					m_scene->properties().backgroundMode().setIndex(BackgroundMode_Environment);
					m_scene->properties().environment().texture()->fileName().set(path);
					m_scene->undoStack().endMacro();
				});

				QTimer::singleShot(10, this, [this] {
					m_menu->popup(QCursor::pos());
				});
				return true;
			}
			break;

		case DragType_Image:
			if (m_ctx && m_scene) {
				auto geom = m_ctx->renderer->getGeometry(x, y);
				m_menu.reset(new QMenu(this));

				if (geom) {
					auto * material = geom->material();
					if (m_scene->materialManager().isDefault(material)) {
						m_menu->addAction("Layer/Diffuse Color", [this, geom, path]() {
							m_scene->undoStack().beginMacro("Set diffuse texture");
							auto * newMaterial = m_scene->materialManager().create(QFileInfo(path).baseName());
							newMaterial->group(0)->layer(0)->diffuseColor().texture()->fileName().set(path);
							m_scene->setMaterial({ geom }, newMaterial);
							m_ctx->materialInfo->setMaterial(newMaterial);
							m_scene->undoStack().endMacro();
						});
					} else {
						for (size_t gi = 0; gi < material->numGroups(); gi++) {
							auto * group = material->group(gi);
							for (size_t li = 0; li < group->numLayers(); ++li) {
								auto * layer = group->layer(li);
								const auto layerName = layer->name().get();
								if (layer->diffuseLayer().get()) {
									m_menu->addAction(QString("%1/Diffuse Color").arg(layerName), [this, layer, path]() {
										layer->diffuseColor().texture()->fileName().set(path);
									});
								}
							}
						}
					}
				}

				if (m_menu->actions().count() > 0)
					m_menu->addSeparator();

				m_menu->addSeparator();

				m_menu->addAction("Background", [this, path]() {
					m_scene->undoStack().beginMacro("Set background");
					auto & sceneProperties = m_scene->properties();
					sceneProperties.backgroundMode().setIndex(BackgroundMode_PlaneImage);
					sceneProperties.background().texture()->fileName().set(path);
					m_scene->undoStack().endMacro();
				});

				QTimer::singleShot(10, this, [this] {
					m_menu->popup(QCursor::pos());
				});

				return true;
			}
			break;

		default: break;
	}

	return false;
}

bool MainWidget::isImageURL(const QString & url) const
{
	if (url == "file://")
		return true;
	return m_appCtx.imageManager()->isImageFile(::urlToLocalFile(url));
}

QString MainWidget::urlToLocalFile(const QVariant & value) const
{
	return m_appCtx.getShortResourcePath(::urlToLocalFile(value));
}

void MainWidget::fetchSettings()
{
	if (m_ctx && m_ctx->renderer) {
		m_ctx->renderer->setEnableDenoise(m_settings.value(SETTINGS_PREVIEW_DENOISE, PREVIEW_DENOISE_DEFAULT).toBool());
		m_ctx->renderer->setMaxIterations(m_settings.value(SETTINGS_PREVIEW_FRAMES, PREVIEW_FRAMES_DEFAULT).toInt());
	}
}

static NodeSelection getCommonNodeSelection(const NodeSelection & nodeSelection)
{
	NodeSelection commonNodeSelection;
	for (auto node : nodeSelection) {
		if (node->type() == SceneElement_Node || node->type() == SceneElement_MeshNode)
			commonNodeSelection.push_back(node);
	}
	return commonNodeSelection;
}

bool MainWidget::getUndoEnabled() const
{
	return m_scene ? m_scene->undoStack().canUndo() : false;
}

bool MainWidget::getRedoEnabled() const
{
	return m_scene ? m_scene->undoStack().canRedo() : false;
}

bool MainWidget::canOpenMetadataEditor() const
{
	return m_scene ? m_scene->nodeSelection().size() == 1 : false;
}

void MainWidget::beginRenderingParameters()
{
	if (m_scene) {
		LOCK_MENU
		m_renderingParameters.reset(new RenderingParametersProxy(m_settings, rootContext(), *m_core, m_appCtx, this));
	}
}

void MainWidget::acceptRenderingParameters()
{
	if (m_renderingParameters) {
		m_renderingParameters->accept();
		m_activeRenderOptions = m_renderingParameters->options();
		QVariant retVal;
		QMetaObject::invokeMethod(rootObject(), "runRendering", Q_RETURN_ARG(QVariant, retVal));
	}
}

void MainWidget::endRenderingParameters()
{
	UNLOCK_MENU
	m_renderingParameters.reset();
}

void MainWidget::beginRendering()
{
	LOCK_MENU
	m_rendering.reset(new RenderingProxy(m_settings, rootContext(), m_activeRenderOptions, m_imageProvider, this));
}

void MainWidget::endRendering()
{
	UNLOCK_MENU
	m_rendering.reset();
}

void MainWidget::beginFitToView()
{
	if (m_scene) {
		LOCK_MENU
		m_fitToView.reset(new FitToViewProxy(m_scene->camera().beginFitToView(), rootContext()));
	}
}

void MainWidget::acceptFitToView()
{
	if (m_fitToView)
		m_fitToView->accept();
}

void MainWidget::endFitToView()
{
	UNLOCK_MENU
	m_fitToView.reset();
}

void MainWidget::dropToSurface()
{
	if (m_scene) {
		auto nodeSelection = getCommonNodeSelection(m_scene->nodeSelection());
		if (!nodeSelection.empty())
			m_scene->beginDropToSurface(nodeSelection);
	}
}

void MainWidget::beginEditNormals()
{
	if (m_scene) {
		LOCK_MENU
		m_editNormals.reset(new EditNormalsProxy(m_scene->beginEditNormals(m_scene->geomSelection()), rootContext()));
	}
}

void MainWidget::acceptEditNormals()
{
	if (m_editNormals)
		m_editNormals->accept();
}

void MainWidget::endEditNormals()
{
	UNLOCK_MENU
	m_editNormals.reset();
}

void MainWidget::beginUVMapping()
{
	if (m_scene) {
		LOCK_MENU
		m_uvMapping.reset(new UVMappingProxy(m_scene->beginUVMapping(m_scene->geomSelection()), rootContext()));
	}
}

void MainWidget::acceptUVMapping()
{
	if (m_uvMapping)
		m_uvMapping->accept();
}

void MainWidget::endUVMapping()
{
	UNLOCK_MENU
	m_uvMapping.reset();
}

void MainWidget::beginPivotParameters()
{
	if (m_scene) {
		LOCK_MENU
		m_pivotParameters.reset(new PivotParametersProxy(m_scene->beginPivotParameters(m_scene->nodeSelection()), rootContext()));
	}
}

void MainWidget::acceptPivotParameters()
{
	if (m_pivotParameters)
		m_pivotParameters->accept();
}

void MainWidget::endPivotParameters()
{
	UNLOCK_MENU
	m_pivotParameters.reset();
}

void MainWidget::beginNodeMetadata() {}

void MainWidget::beginResourceManager()
{
	if (m_scene) {
		LOCK_MENU
		m_resourceManager.reset(new ResourceManagerProxy(this, *m_scene));
		rootContext()->setContextProperty("resManagerPresenter", m_resourceManager.get());
	}
}

void MainWidget::endResourceManager()
{
	UNLOCK_MENU
	rootContext()->setContextProperty("resManagerPresenter", nullptr);
	m_resourceManager.reset();
}

void MainWidget::beginRenderingLayersManager()
{
	if (m_scene) {
		LOCK_MENU
		m_renderingLayersManager.reset(new RenderingLayersManagerProxy(m_scene->renderLayerManager(), this));
		rootContext()->setContextProperty("renderingLayersManagerPresenter", m_renderingLayersManager.get());
	}
}

void MainWidget::endRenderingLayersManager()
{
	UNLOCK_MENU
	rootContext()->setContextProperty("renderingLayersManagerPresenter", nullptr);
	m_renderingLayersManager.reset();
}

void MainWidget::beginAppSettings()
{
	LOCK_MENU
	m_appSettings.reset(new AppSettingsProxy(*m_appCtx.settings(), rootContext()));
}

void MainWidget::acceptAppSettings()
{
	if (m_appSettings)
		m_appSettings->accept();
}

void MainWidget::endAppSettings()
{
	UNLOCK_MENU
	m_appSettings.reset();
	fetchSettings();
}

void MainWidget::beginAbout() {}
void MainWidget::beginTextureDialog()
{
	LOCK_MENU
}

void MainWidget::endTextureDialog()
{
	UNLOCK_MENU
}

static bool isAnyNodeVisible(const NodeSelection & nodeSelection)
{
	for (auto node : nodeSelection) {
		if (node->visible().get())
			return true;
	}
	return false;
}

void MainWidget::onNodeChanged(const INode *, eNodeChanged nodeChanged)
{
	if (m_scene && nodeChanged == NodeChanged_Visibility)
		m_editHideShow->setText(!m_editHideShow->isEnabled() || isAnyNodeVisible(m_scene->nodeSelection()) ? "Hide Selection" : "Show Selection");
}

void MainWidget::onSelectionChanged()
{
	m_hasSelection = m_scene != nullptr && !m_scene->selection().empty();
	const auto nodeSelection = m_scene != nullptr ? m_scene->nodeSelection() : NodeSelection();
	const auto geomSelection = m_scene != nullptr ? m_scene->geomSelection() : GeomSelection();
	const auto commonNodeSelection = getCommonNodeSelection(nodeSelection);
	m_hasNodeSelection = !nodeSelection.empty();
	m_hasGeomSelection = !geomSelection.empty();
	m_hasCommonNodeSelection = !commonNodeSelection.empty();

	m_sceneExtractMeshes->setEnabled(m_hasGeomSelection);
	m_canCombineMeshes = geomSelection.size() > 1;
	m_sceneCombineMeshes->setEnabled(m_canCombineMeshes);
	m_canGroupSelected = commonNodeSelection.size() > 1;
	m_sceneGroupSelected->setEnabled(m_canGroupSelected);
	// m_canOpenMetadataEditor = nodeSelection.size() == 1;
	m_toolsDropToSurface->setEnabled(m_hasCommonNodeSelection);
	m_toolsUVMapping->setEnabled(m_hasGeomSelection);
	m_toolsEditNormals->setEnabled(m_hasGeomSelection);
	m_toolsPivotParameters->setEnabled(m_hasCommonNodeSelection);

	m_editCut->setEnabled(m_hasSelection);
	m_editCopy->setEnabled(m_hasSelection);
	m_editDelete->setEnabled(m_hasSelection);
	m_editHideShow->setEnabled(m_hasSelection);
	m_editHideShow->setText(!m_editHideShow->isEnabled() || isAnyNodeVisible(nodeSelection) ? "Hide Selection" : "Show Selection");

	if (m_scene && m_ctx) {
		auto node = nodeSelection.size() == 1 ? nodeSelection.front() : nullptr;
		if (nodeSelection.empty()) {
			for (auto & geom : geomSelection) {
				if (!node) {
					node = geom->parent();
				} else if (node != geom->parent()) {
					node = nullptr;
					break;
				}
			}
		}
		m_ctx->nodeInfo = node ? NodeInfo::create(*node) : nullptr;

		IMaterial * material = nullptr;
		for (auto geom : geomSelection) {
			if (!material) {
				material = geom->material();
			} else if (material != geom->material()) {
				material = nullptr;
				break;
			}
		}

		m_ctx->materialInfo->setMaterial(material); // set current material
	}

	if (m_ctx && m_ctx->nodeInfo)
		rootContext()->setContextProperty("nodeModel", m_ctx->nodeInfo->modelData());
	else
		rootContext()->setContextProperty("nodeModel", nullptr);

	emit gizmoChanged();
	emit selectionChanged();
}

// ------------------------------------------------------------------------ //

void MainWidget::editCut()
{
	editCopy();
	editDelete();
}

void MainWidget::editCopy()
{
	if (m_scene) {
		auto nodeSelection = m_scene->nodeSelection();
		if (!nodeSelection.empty()) {
			const auto data = m_scene->copyNodes(nodeSelection);
			auto * mimeData = new QMimeData();
			mimeData->setData(CLIPBOARD_MIME_TYPE, data);
			QApplication::clipboard()->setMimeData(mimeData);
		}
	}
}

void MainWidget::editPaste()
{
	if (m_scene) {
		const auto * mimeData = QApplication::clipboard()->mimeData();
		if (mimeData && mimeData->hasFormat(CLIPBOARD_MIME_TYPE)) {
			const auto data = mimeData->data(CLIPBOARD_MIME_TYPE);
			const auto nodeSelection = m_scene->nodeSelection();
			((nodeSelection.size() == 1) ? nodeSelection[0] : &m_scene->root())->pasteNodes(data);
		}
	}
}

void MainWidget::editDelete()
{
	if (m_scene) {
		auto selection = m_scene->selection();
		if (!selection.empty())
			m_scene->deleteElements(selection);
	}
}

void MainWidget::editShowHide()
{
	if (m_scene) {
		auto nodeSelection = m_scene->nodeSelection();
		m_scene->setVisible(nodeSelection, !isAnyNodeVisible(nodeSelection));
	}
}

void MainWidget::undo()
{
	if (m_scene)
		m_scene->undoStack().undo();
}

void MainWidget::redo()
{
	if (m_scene)
		m_scene->undoStack().redo();
}

void MainWidget::extractMeshes()
{
	if (m_scene) {
		auto geomSelection = m_scene->geomSelection();
		if (!geomSelection.empty())
			m_scene->moveMeshes(geomSelection, false, &m_scene->root());
	}
}

void MainWidget::combineMeshes()
{
	if (m_scene) {
		auto geomSelection = m_scene->geomSelection();
		if (!geomSelection.empty()) {
			auto newMesh = m_scene->mergeMeshes(geomSelection);
			assert(newMesh);
			m_scene->setSelection({ newMesh }, SelectionOperation_Set);
		}
	}
}

void MainWidget::groupSelected()
{
	if (m_scene) {
		auto nodeSelection = getCommonNodeSelection(m_scene->nodeSelection());
		if (!nodeSelection.empty())
			m_scene->moveNodes(nodeSelection, nullptr, (size_t)-1, "Group");
	}
}

// ------------------------------------------------------------------------ //

void MainWidget::onUndoChanged()
{
	emit undoChanged();
}

void MainWidget::onClipboardChanged()
{
	auto mime = QApplication::clipboard()->mimeData();
	m_hasCopiedNodes = mime && mime->hasFormat("org.miray/copied-nodes");
	emit clipboardChanged();
}

// ------------------------------------------------------------------------ //

void MainWidget::onProgressStarted(const QString & operation)
{
//	qDebug() << "onProgressStarted" << operation;
	QVariant retVal;
	QMetaObject::invokeMethod(rootObject(), "beginProgress", Q_RETURN_ARG(QVariant, retVal), Q_ARG(QVariant, operation));
}

void MainWidget::onProgressUpdated(float progress)
{
	QVariant retVal;
	QMetaObject::invokeMethod(rootObject(), "updateProgress", Q_RETURN_ARG(QVariant, retVal), Q_ARG(QVariant, progress));
}

void MainWidget::onProgressFinished()
{
//	qDebug() << "onProgressFinished";
	QVariant retVal;
	QMetaObject::invokeMethod(rootObject(), "endProgress", Q_RETURN_ARG(QVariant, retVal));
}

void MainWidget::onSceneLoaded()
{
	if (m_scene && m_scene->root().numChildren(SceneElement_All) == 1)
		m_scene->setSelection({ m_scene->root().child(0, SceneElement_All) }, SelectionOperation_Set);
}

void MainWidget::onSceneSaved()
{
	qDebug() << "Scene saved to" << m_savingFilePath << "(" << m_savingTmlFilePath << ")";
	if (QFile::exists(m_savingFilePath))
		QFile::remove(m_savingFilePath);
	QFile::rename(m_savingTmlFilePath, m_savingFilePath);
	m_savingFilePath.clear();
	m_savingTmlFilePath.clear();

	if (m_clearUndoAfterSave && m_scene)
		m_scene->undoStack().clear();
}

void MainWidget::onSceneLoadingFailed(const QString & error)
{
	qDebug() << "Scene loading failed:" << error;
	if (!error.isEmpty()) {
		QMessageBox::warning(this, "MiRay", "Scene loading error:\n\n" + error);
	}
}

void MainWidget::onSceneSavingFailed(const QString & error)
{
	qDebug() << "Scene saving failed:" << error;
}

int MainWidget::getGizmo() const
{
	return m_ctx && m_ctx->renderer ? m_ctx->renderer->getGizmo() : -1;
}

void MainWidget::setGizmo(int mode)
{
	if (m_ctx && m_ctx->renderer) {
		m_ctx->renderer->setGizmo((eGizmo)mode);
		if (mode >= 0 && mode < m_sceneGizmo->actions().size())
			m_sceneGizmo->actions()[mode]->setChecked(true);
		emit gizmoChanged();
	}
}

QImage MainWidget::currentMaterialPreview() const {
	if (m_ctx && m_ctx->materialInfo) {
		return m_ctx->materialInfo->getCurrentMaterialPreview();
	}
	return QImage();
}
