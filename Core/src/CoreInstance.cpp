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

#include "CoreInstance.h"
#include "CoreRenderer.h"
#include "Renderers/VolumePathTracer.h"
#include "Renderers/UPBP/UPBP.h"
#include "MiRaySceneLoader.h"
#include "../../Shared/Interfaces/SerializationContext.h"
#include "../../Shared/Interfaces/Settings.h"

#include <OpenImageDenoise/oidn.hpp>

void oidnErrorCallback(void* userPtr, oidn::Error error, const char* message)
{
	static const char * errors[] = {
		"none",
		"unknown",
		"invalid argument",
		"invalid operation",
		"out of memory",
		"unsupported hardware",
		"cancelled"
	};
	LogError() << QString().asprintf("OpenImageDenoise (%s) error: %s", errors[static_cast<int>(error)], message ? message : "");
}

CoreInstance::CoreInstance(IApplicationContext &ctx)
	: m_ctx(ctx)
	, m_renderer(nullptr)
	, m_loadedWell(false)
	, m_savedWell(false)
{
	// LogInformation() << "Creating Renderers";
	m_scene = std::make_unique<Scene>(ctx, *this);

	m_renderers[RenderingAlgorithm_VolumePathTracing] = std::make_unique<VolumePathTracer>(*m_scene);
	m_renderers[RenderingAlgorithm_UPBP] = std::make_unique<UPBP>(*m_scene,
													 UPBP::kCustom,		// Algorithm
													 BPT | SURF | PP3D | PB2D,	// EstimatorTechniques
													 CONSTANT_RADIUS,	// PB2DRadiusCalculation
													 0,					// PB2DRadiusKNN
													 SHORT_BEAM,		// QueryBeamType
													 CONSTANT_RADIUS,	// BB1DRadiusCalculation
													 0,					// BB1DRadiusKNN
													 SHORT_BEAM,		// PhotonBeamType
													 0.f,				// BB1DBeamStorageFactor
													 0x40000,			// RefPathCountPerIter
													 0x40000,			// PathCountPerIter
													 0.f,				// MinDistToMed
													 1024*1024*1024);	// MaxMemoryPerThread

	connect(m_ctx.imageManager(), SIGNAL(beforeImageUpdate(const QString&)), this, SLOT(onBeforeImageUpdate(const QString&)), Qt::DirectConnection);
	connect(m_ctx.imageManager(), SIGNAL(afterImageUpdate(const QString&)), this, SLOT(onAfterImageUpdate(const QString&)), Qt::DirectConnection);
}

void CoreInstance::initOIDN()
{
	if (m_oidnDevice)
		return;

#if defined(__APPLE__)
	LogInformation() << "Attempting to initialize OpenImageDenoise Metal device...";
	m_oidnDevice = oidn::newDevice(oidn::DeviceType::Metal);
	if (m_oidnDevice) {
		const char* errorMessage = nullptr;
		auto error = m_oidnDevice.getError(errorMessage);
		if (error != oidn::Error::None) {
			m_oidnDevice = oidn::DeviceRef();
		}
	}
#endif

	if (!m_oidnDevice) {
		LogInformation() << "Initializing OpenImageDenoise default device...";
		m_oidnDevice = oidn::newDevice(oidn::DeviceType::Default);
	} else {
		LogInformation() << "Successfully initialized OpenImageDenoise Metal device.";
	}
	if (m_oidnDevice) {
		const char* errorMessage = nullptr;
		auto error = m_oidnDevice.getError(errorMessage);
		if (error != oidn::Error::None) {
			oidnErrorCallback(this, error, errorMessage);
			m_oidnDevice = oidn::DeviceRef();
		} else {
			m_oidnDevice.setErrorFunction(oidnErrorCallback, this);
			m_oidnDevice.commit();

			// Check if the device supports system memory sharing (e.g. CPU vs GPU/Metal)
			if (!m_oidnDevice.get<bool>("systemMemorySupported")) {
				LogInformation() << "Default OpenImageDenoise device does not support system memory sharing. Falling back to CPU.";
				m_oidnDevice = oidn::newDevice(oidn::DeviceType::CPU);
				if (m_oidnDevice) {
					m_oidnDevice.setErrorFunction(oidnErrorCallback, this);
					m_oidnDevice.commit();
				}
			}

			if (m_oidnDevice) {
				LogInformation() << QString("OpenImageDenoise version %1.%2.%3")
					.arg(m_oidnDevice.get<int>("versionMajor"))
					.arg(m_oidnDevice.get<int>("versionMinor"))
					.arg(m_oidnDevice.get<int>("versionPatch"));
				m_oidnFilter = m_oidnDevice.newFilter("RT");
			}
		}
	}
}

CoreInstance::~CoreInstance()
{
	// qDebug() << "~CoreInstance";
	if (m_loading.isRunning()) {
		LogWarning() << "Loading is still running, let's wait";
		m_loading.cancel();
		m_loading.waitForFinished();
		LogWarning() << "Loading is done, can delete renderer now";
	}

	if (m_saving.isRunning()) {
		LogWarning() << "Saving is still running, let's wait";
		m_saving.cancel();
		m_saving.waitForFinished();
		LogWarning() << "Saving is done, can delete renderer now";
	}

	m_importCommand.reset();
}

ILog &CoreInstance::log()
{
	return *m_ctx.log();
}

ICoreRenderer *CoreInstance::createRenderer(float devicePixelRatio, bool offscreen)
{
	if (!m_renderer)
		m_renderer = new CoreRenderer(*this, devicePixelRatio, offscreen); // will be deleted by its new owner, not here

	return m_renderer;
}

ICoreRenderer *CoreInstance::renderWidget() const
{
	return m_renderer;
}

IScene *CoreInstance::getScene() const
{
	return m_scene.get();
}

bool CoreInstance::loadFile(const QString &fileName, const LoadingParameters &params)
{
	auto fullFileName = m_ctx.getFullResourcePath(fileName);

	LogInformation() << "Core is about to" << (params.newScene ? "load" : "import") << "scene: " << fullFileName;

	auto loadersReg = m_ctx.modelLoadersRegistry();
	if (!loadersReg->canLoadFile(fullFileName, true))
		throw std::runtime_error(QString("Error loading '%1': Format is not supported").arg(QFileInfo(fullFileName).fileName()).toStdString());

	ModelLoadingContext ctx;
	ctx.loadNewScene = params.newScene;
	ctx.putOnTheFloor = m_ctx.settings()->getValue(SETTINGS_PUT_LOADED_MODELS_ON_THE_FLOOR, DEFAULT_PUT_LOADED_MODELS_ON_THE_FLOOR).toBool();
	ctx.sourceFileName = fullFileName;
	ctx.sourceFolder = nativePath(QFileInfo(fullFileName).absolutePath());
	ctx.fileVersion = 0;

	LogInformation() << "Loading started";

	m_scene->lock(SceneInternalModification_Import);
	if (params.newScene) {
		m_fileName = fullFileName;
		m_scene->reset();
	} else {
		assert(!m_importCommand);
		m_importCommand.reset(new SceneImportCommand(*m_scene, m_scene->root()));
	}

	// the rest of the loading should be done in a background thread
	m_loading = QtConcurrent::run(this, &CoreInstance::backgroundLoad, ctx);

	if (!params.waitUntilDone) return true;

	m_loading.waitForFinished();
	return m_loadedWell;
}

class MaterialCreator : public IMaterialCreator
{
	MaterialManager & m_materialManager;
	std::map<QString, IMaterial *>	m_materials;

public:
	MaterialCreator(MaterialManager & mm) : m_materialManager(mm) {}

	IMaterial *create(const QString &name, const QJsonObject & params, const ModelLoadingContext &ctx) override
	{
		auto it = m_materials.find(name);
		if (it != m_materials.end())
			return it->second;

		auto material = m_materialManager._create(name, params, ctx);
		m_materials[name] = material;
		return material;
	}

	IMaterial *create(const QString &name, const QDomElement & data, const ModelLoadingContext &ctx) override
	{
		auto it = m_materials.find(name);
		if (it != m_materials.end())
			return it->second;

		auto material = m_materialManager._create(name, data, ctx);
		m_materials[name] = material;
		return material;
	}
};

bool CoreInstance::onProgress(double progress)
{
	emit progressUpdated(static_cast<float>(progress));
	return !m_loading.isCanceled();
}

void CoreInstance::backgroundLoad(ModelLoadingContext ctx)
{
	while (!m_loading.isRunning()) // make sure m_loading is initialized
		QThread::msleep(10);

	emit progressStarted("Loading file");

	try {
		auto loadersReg = m_ctx.modelLoadersRegistry();
		auto loader = loadersReg->getLoader(ctx.sourceFileName);

		auto & materialManager = static_cast<MaterialManager &>(m_scene->materialManager());
		std::unique_ptr<IMaterialCreator> mc(new MaterialCreator(materialManager));

		auto * callbackLoader = qobject_cast<ICallbackModelLoader *>(loader.get());
		if (callbackLoader) {
			callbackLoader->load(ctx, m_scene.get(), mc.get(), this);
		} else {
			auto * stepsLoader = qobject_cast<ISteppedModelLoader *>(loader.get());
			if (stepsLoader) {
				stepsLoader->preLoad(ctx, m_scene.get());

				auto numSteps = stepsLoader->stepCount();
				for (int i = 0; i < numSteps && !m_loading.isCanceled(); i++) {
					stepsLoader->loadStep(ctx, i, mc.get());
					emit progressUpdated(static_cast<float>(i) / numSteps);
				}
			}
		}

		m_scene->buildCollisionScene();

		bool needsSetup = loader->needsAutoSetup();
		if (needsSetup) {
			if (!m_importCommand)
				m_scene->camera().reset();

			auto & bbox = m_scene->root().oobb();
			float radius = std::max(glm::length(bbox.size()) * 5, 1000.f); // use 5 diagonals of the bbox as a radius to make sure it is big enough (10 times more)
			m_scene->properties().environmentSize()._set(radius);
		}

		materialManager.fireMaterialListChanged();
	} catch (const std::exception &ex) {
		LogError() << "Error loading file: " << ex.what();
		emit progressFinished();
		emit sceneLoadingFailed(QString::fromUtf8(ex.what()));
		if (m_importCommand) {
			m_scene->pushCommand(m_importCommand.take());
		} else {
			m_fileName.clear();
		}
		m_scene->unlock(SceneInternalModification_Import);
		return;
	}

	emit progressUpdated(1.f);
	emit progressFinished();

	if (!m_loading.isCanceled()) {
		LogInformation() << "Loading is done";
		m_loadedWell = true;
	} else {
		LogWarning() << "Loading has been canceled, file has not been loaded";
		emit sceneLoadingFailed("");
	}

	if (m_renderer)
		m_renderer->onLoadingDone();

	if (m_importCommand) {
		m_scene->pushCommand(m_importCommand.take());
	}

	m_scene->unlock(SceneInternalModification_Import);

	emit sceneLoaded();
}

void CoreInstance::saveFile(const QString &fileName, const SavingParameters &params)
{
	auto savers = m_ctx.modelSaversRegistry();
	if (!savers->canSaveFile(fileName))
		throw std::runtime_error(QString("Error saving '%1': Format is not supported").arg(QFileInfo(fileName).fileName()).toStdString());

	LogInformation() << QString("Start saving %1").arg(fileName);

	ModelSavingContext ctx;
	ctx.targetFileName = fileName;
	ctx.targetFolder = nativePath(QFileInfo(fileName).absolutePath());

	if (params.collectResources) {
		QSet<QString> files;
		m_scene->collectFileNames(files);
		ctx.mapFileNamesOverride = generateUniqueFileNamesForResourcesCollection(files, params.collectionBlacklist);
	}

	if (params.storeFileName) {
		m_fileName = fileName;
	}

	// the rest of the saving should be done in a background thread
	m_savedWell = false;
	m_saving = QtConcurrent::run(this, &CoreInstance::backgroundSave, ctx);

	if (params.waitUntilDone)
		m_saving.waitForFinished();

	if (!m_savedWell)
		throw std::runtime_error(m_savingError.toStdString());
}

void CoreInstance::backgroundSave(ModelSavingContext ctx)
{
	emit progressStarted("Saving file");

	try {
		auto savers = m_ctx.modelSaversRegistry();
		auto saver = savers->getSaver(ctx.targetFileName);
		connect(saver.get(), SIGNAL(updateProgress(float)), this, SLOT(updateSaverProgress(float)), Qt::DirectConnection);

		m_savedWell = saver->save(ctx, m_scene.get());
	} catch (const std::exception &ex) {
		LogError() << "Error saving file: " << ex.what();
		emit progressFinished();
		emit sceneSavingFailed(QString::fromUtf8(ex.what()));
		m_savingError = QString::fromUtf8(ex.what());
		return;
	}

	QStringList failedFiles;

	if (!m_saving.isCanceled()) {
		emit progressUpdated(0.9f);

		for (auto it = ctx.mapFileNamesOverride.begin(); it != ctx.mapFileNamesOverride.end() && !m_saving.isCanceled(); it++) {
			auto src = it.key();
			auto dst = nativePath(QString("%1/%2").arg(ctx.targetFolder, it.value()));

			LogInformation() << QString("About to copy '%1' to '%2'").arg(src).arg(dst);

			if (m_ctx.isInternalResource(src))
				src = m_ctx.getFullResourcePath(src);

			try {
				if (QFile::exists(dst) && !QFile::remove(dst))
					throw std::runtime_error(QString("File '%1' already exists and cannot be deleted for further overwriting").arg(dst).toStdString());
				if (!QFile::copy(src, dst))
					throw std::runtime_error(QString("Unable to copy file '%1' to '%2'").arg(src).arg(dst).toStdString());

				LogInformation() << "  - OK";
			} catch (const std::exception &ex) {
				LogError() << QString("  - ERROR: %1").arg(ex.what());
				failedFiles.append(QString::fromUtf8(ex.what()));
				m_savedWell = false;
			}
		}
	}

	emit progressUpdated(1.f);
	emit progressFinished();

	if (!failedFiles.isEmpty()) {
		m_savingError = failedFiles.join("\n");
		m_savedWell = false;
		return;
	}

	if (m_saving.isCanceled()) {
		LogWarning() << "Saving has been canceled, file has not been saved";
		emit sceneSavingFailed("");
	} else {
		LogInformation() << "File has been saved";
		emit sceneSaved();
	}
}

void CoreInstance::updateSaverProgress(float progress)
{
	LogInformation() << QString().asprintf("Saving: %.1f%%", progress * 90.f);
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	emit progressUpdated(progress);
}

void CoreInstance::cancelProgress()
{
	if (m_loading.isRunning())
		m_loading.cancel();

	if (m_saving.isRunning())
		m_saving.cancel();
}

void CoreInstance::onBeforeImageUpdate(const QString&)
{
	m_scene->lock(SceneInternalModification_Texture);
}

void CoreInstance::onAfterImageUpdate(const QString&)
{
	m_scene->unlock(SceneInternalModification_Texture);
}

QString CoreInstance::fileName() const { return m_fileName; }

void CoreInstance::createRenderingBuffers(RenderingBuffers & buffers, int width, int height, uint32_t bufferTypes) const
{
	auto * imageManager = m_ctx.imageManager();

	buffers.frameBuffer = imageManager->createImage(width, height, eImageFormat::RGBA, eImageDataType::Float, eImageColorSpace::Linear);
	buffers.denoisedBuffer.reset();
	buffers.normalBuffer.reset();
	buffers.depthBuffer.reset();
	buffers.albedoBuffer.reset();
	buffers.objectsBuffer.reset();
	buffers.materialsBuffer.reset();
	buffers.layerBuffers.clear();

	if (bufferTypes & BufferType_Denoised)
		buffers.denoisedBuffer = imageManager->createImage(width, height, eImageFormat::RGBA, eImageDataType::Float, eImageColorSpace::Linear);

	if (bufferTypes & (BufferType_Normal | BufferType_Denoised))
		buffers.normalBuffer = imageManager->createImage(width, height, eImageFormat::RGB, eImageDataType::Float, eImageColorSpace::Linear);

	if (bufferTypes & BufferType_Depth)
		buffers.depthBuffer = imageManager->createImage(width, height, eImageFormat::Grayscale, eImageDataType::Float, eImageColorSpace::Linear);

	if (bufferTypes & (BufferType_Albedo | BufferType_Denoised))
		buffers.albedoBuffer = imageManager->createImage(width, height, eImageFormat::RGB, eImageDataType::Float, eImageColorSpace::Linear);

	if (bufferTypes & BufferType_Objects)
		buffers.objectsBuffer = imageManager->createImage(width, height, eImageFormat::RGB, eImageDataType::Float, eImageColorSpace::Linear);

	if (bufferTypes & BufferType_Materials)
		buffers.materialsBuffer = imageManager->createImage(width, height, eImageFormat::RGB, eImageDataType::Float, eImageColorSpace::Linear);

	if (bufferTypes & BufferType_Layers) {
		auto & renderLayerManager = m_scene->renderLayerManager();
		for (int i = -1; i < renderLayerManager.count(); i++) {
			std::string name = renderLayerManager.get(i)->name().get().toUtf8().data();
			buffers.layerBuffers.emplace_back(std::make_pair(name, imageManager->createImage(width, height, eImageFormat::RGBA, eImageDataType::Float, eImageColorSpace::Linear)));
		}
	}
}

bool CoreInstance::denoise(const RenderingBuffers & buffers)
{
	assert(buffers.denoisedBuffer.get() != buffers.frameBuffer.get());
	if (buffers.denoisedBuffer.get() == buffers.frameBuffer.get())
		return false;

	const auto width = buffers.frameBuffer->width();
	const auto height = buffers.frameBuffer->height();
	assert(buffers.denoisedBuffer && buffers.denoisedBuffer->width() == width && buffers.denoisedBuffer->height() == height);
	assert(buffers.denoisedBuffer->dataType() == eImageDataType::Float && buffers.denoisedBuffer->format() == eImageFormat::RGBA);
	initOIDN();
	if (m_oidnFilter) {
		m_oidnFilter.setImage("color", (void *)buffers.frameBuffer->rawData(), oidn::Format::Float3, width, height, 0, 16, 16 * width);
		if (buffers.albedoBuffer)
			m_oidnFilter.setImage("albedo", (void *)buffers.albedoBuffer->rawData(), oidn::Format::Float3, width, height, 0, 12, 12 * width);
		if (buffers.normalBuffer)
			m_oidnFilter.setImage("normal", (void *)buffers.normalBuffer->rawData(), oidn::Format::Float3, width, height, 0, 12, 12 * width);
		m_oidnFilter.setImage("output", (void *)buffers.denoisedBuffer->rawData(), oidn::Format::Float3, width, height, 0, 16, 16 * width);
		m_oidnFilter.set("hdr", true);
		m_oidnFilter.commit();
		m_oidnFilter.execute();
		return true;
	}
	return false;
}

int CoreInstance::render(const RenderingBuffers &buffers, const RenderingParameters &params, int iterationCount, int startIteration, volatile bool &stop)
{
	auto renderer = m_renderers[params.algorithm].get();
	assert(renderer);

	std::vector<IImage *> layerBuffers(buffers.layerBuffers.size());
	for (size_t i = 0; i < layerBuffers.size(); i++)
		layerBuffers[i] = buffers.layerBuffers[i].second.get();

	stop = false;
	Random random;
	int i = 0;
	for (; i < iterationCount && !stop; i++) {
		renderer->runIteration(*buffers.frameBuffer.get(), buffers.albedoBuffer.get(), buffers.normalBuffer.get(), buffers.depthBuffer.get(),
								buffers.objectsBuffer.get(), buffers.materialsBuffer.get(), &layerBuffers,
								params.renderArea, vec2(random.generate1D(), random.generate1D()), startIteration + i,
								false, params.maxIntensity, params.photonScale);
	}
	return i;
}
