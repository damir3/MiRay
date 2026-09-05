#include "PreviewRenderer.h"

#include <QtConcurrent/QtConcurrent>
#include <QUndoStack>

#include "../Shared/Interfaces/ApplicationContext.h"
#include "../Shared/Interfaces/Material.h"
#include "../Shared/Interfaces/MaterialManager.h"
#include "../Shared/Interfaces/SerializationContext.h"

constexpr int PREVIEW_WIDTH = 256;
constexpr int PREVIEW_HEIGHT = 256;
constexpr int PREVIEW_PASSES = 100;
constexpr int PASSES_PER_STEP = 10;
constexpr float MAX_RENDERING_INTENSITY = 100.f;

PreviewRenderer::PreviewRenderer(IApplicationContext & appCtx)
	: m_appCtx(appCtx)
{
	auto previews = m_appCtx.getResourcesFolder();
	previews.cd("Preview");

	auto scenes = previews.entryList(QStringList("*.mirayScene"));
	for (auto s : scenes)
		m_sceneNames.append(QFileInfo(s).completeBaseName());

	m_sceneNames.sort();

	assert(m_sceneNames.contains("Ball"));
	setScene("Ball");
}

PreviewRenderer::~PreviewRenderer()
{
	stop();
}

void PreviewRenderer::setScene(const QString & name)
{
	if (!getScenes().contains(name)) {
		assert(false && "Unknown scene");
		return;
	}

	stop();

	m_passCount = 0;
	m_sceneName = name;

	m_core.reset(m_appCtx.createCoreInstance());
	if (!m_buffers.frameBuffer) {
		m_core->createRenderingBuffers(m_buffers, PREVIEW_WIDTH, PREVIEW_HEIGHT, BufferType_Denoised);
	}

	auto path = QString("miray:///Preview/%1.mirayScene").arg(m_sceneName);
	if (!m_core->loadFile(path, LoadingParameters::forSceneLoading())) {
		m_appCtx.log()->error(QString("PreviewRenderer: can't load scene '%1'").arg(m_sceneName));
	}

	m_scene = m_core->getScene();
}

void PreviewRenderer::setMaterial(const IMaterial* material)
{
	stop();

	m_originalMaterial = const_cast<IMaterial *>(material);
	m_passCount = 0;
}

void PreviewRenderer::stop()
{
	if (!m_future.isRunning()) return;

	m_stop = true;
	m_future.cancel();
	m_future.waitForFinished();
	emit progressChanged();
	// qDebug() << "stop preview renderer";
}

bool PreviewRenderer::renderStep()
{
	if (!m_originalMaterial || m_future.isRunning() || m_passCount >= PREVIEW_PASSES || !m_core.get()) {
		m_stop = true;
		emit progressChanged();
		return false;
	}

	m_stop = false;
	emit progressChanged();
	m_future = QtConcurrent::run(this, &PreviewRenderer::processMaterial);

	return true;
}

void PreviewRenderer::processMaterial()
{
	if (m_passCount == 0)
		updatePreviewMaterial();

	for (int i = 0; i < PASSES_PER_STEP; i++) {
		m_core->render(m_buffers, { RenderingAlgorithm_VolumePathTracing, MAX_RENDERING_INTENSITY }, 1, m_passCount++, m_stop);
		if (m_stop) return;
	}

	if (m_originalMaterial) {
		m_core->denoise(m_buffers);
		m_originalMaterial->setPreview(m_buffers.denoisedBuffer->toQImage(true));
	}

	emit rendered(m_originalMaterial.data());
	emit progressChanged();
}

void PreviewRenderer::updatePreviewMaterial()
{
	if (!m_originalMaterial) {
		assert(false && "Nothing to load to preview");
		return;
	}

	if (!m_scene) {
		assert(false && "No preview scene loaded");
		return;
	}

	auto & materialManager = m_scene->materialManager();
	auto oldMaterial = materialManager.getByName("Material");
	if (!oldMaterial) {
		assert(false && "Scene must have a 'Material' material in order to be used for preview");
		return;
	}

	auto saved = m_originalMaterial->save({});
	auto newMaterial = materialManager.load(saved, {}, materialManager.count());

	m_scene->replaceMaterial(oldMaterial, newMaterial);
	materialManager.remove({ oldMaterial });
	m_scene->undoStack().clear(); // delete removed material from undo stack

	newMaterial->name()._set("Material");
}

float PreviewRenderer::progress() const
{
	return m_stop ? 1.f : (float)m_passCount / PREVIEW_PASSES;
}
