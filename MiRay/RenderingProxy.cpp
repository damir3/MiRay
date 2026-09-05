#include "RenderingProxy.h"

#include "../Shared/Interfaces/CoreRenderer.h"
#include <../Shared/Interfaces/CoreInstance.h>
#include <QTime>
#include <QElapsedTimer>
#include <QtConcurrent/QtConcurrent>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>

static const QString SETTINGS_LAST_SAVED_IMAGE = "lastSavedImage";

RenderingProxy::RenderingProxy(QSettings & settings, QQmlContext * context, const RenderingContext & ctx, ColorImageProvider * imageProvider, QObject * parent)
	: QObject(parent)
	, m_settings(settings)
	, m_context(context)
	, m_imageProvider(imageProvider)
	, m_ctx(ctx)
	, m_totalIterationCount(0)
	, m_denoised(false)
	, m_stop(false)
{
	uint32_t types = m_ctx.extraChannels ? -1 : 0;
	types = m_ctx.denoise ? (types | BufferType_Denoised) : (types & ~BufferType_Denoised);
	m_ctx.core->createRenderingBuffers(m_renderingBuffers, m_ctx.width, m_ctx.height, types);

	m_imageProvider->setImage(m_renderingBuffers.frameBuffer->toQImage(true));

	m_context->setContextProperty("renderingModel", this);

	emit previewChanged("0");

	m_future = QtConcurrent::run(this, &RenderingProxy::rendering, m_ctx);
}

RenderingProxy::~RenderingProxy()
{
	stop();
	m_context->setContextProperty("renderingModel", nullptr);
}

void RenderingProxy::stop()
{
	m_stop = true;
	m_future.waitForFinished();
}

void RenderingProxy::renderMore()
{
	m_stop = false;
	m_future = QtConcurrent::run(this, &RenderingProxy::rendering, m_ctx);
}

void saveRenderingImage(const QString & fileName, const ImagePtr & image, const IImageManager * imageManager,
						eImageColorSpace colorSpace = eImageColorSpace::Unknown, eAlphaProcessing processing = eAlphaProcessing::None)
{
	ImagePtr imageToSave = image;
	if (colorSpace == eImageColorSpace::sRGB || processing != eAlphaProcessing::None)
		imageToSave = imageManager->convertImage(image, image->format(), image->dataType(), colorSpace, processing);

	QFileInfo fi(fileName);
	const auto data = imageManager->saveImage(imageToSave, fi.suffix(), 1.0f);
	if (data.isEmpty()) {
		throw std::runtime_error(QString("Can't save the image\nFile: %1").arg(fileName).toStdString());
	}
	writeFile(fileName, data);
}

void saveRenderingBuffers(const RenderingBuffers & originalBuffers, bool extraChannels, bool denoised, const QString & fileName, const IImageManager * imageManager)
{
	auto savingFormats = imageManager->savingFormats();

	QFileInfo fi(fileName);

	auto ext = fi.suffix();
	auto savingFormat = std::find_if(savingFormats.begin(), savingFormats.end(), [&ext] (const ImageSavingFormat& v) { return v.extension.compare(ext, Qt::CaseInsensitive) == 0;});

	if (savingFormat == savingFormats.end())
		throw std::invalid_argument("Invalid parameter 'strOutputFileName' passed to method 'saveRenderingBuffers': Unable to find saver for this type of image");

	auto buffers = originalBuffers;
	if (!denoised)
		buffers.denoisedBuffer = nullptr;

	if (buffers.layerBuffers.size() > 1) { // update default layer
		const auto width = buffers.layerBuffers[0].second->width();
		const auto height = buffers.layerBuffers[0].second->height();
		const auto MIN_ALPHA = 0.999f;

		for (int i = 0; i < buffers.layerBuffers.size() - 1; i++) { // create layer buffer copy
			const auto srcImage = buffers.layerBuffers[i].second;
			buffers.layerBuffers[i].second = imageManager->createImage(width, height, srcImage->format(), srcImage->dataType(), srcImage->colorSpace(), srcImage->rawData(), srcImage->dpi());
		}

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				glm::vec4 c = buffers.layerBuffers.back().second->getPixel(x, y);
				for (int i = (int)buffers.layerBuffers.size() - 2; i >= 0 && c.a < MIN_ALPHA; i--) {
					const glm::vec4 cl = buffers.layerBuffers[i].second->getPixel(x, y);
					if (c.a > 0.f && cl.a > 0.f)
						buffers.layerBuffers[i].second->setPixel(x, y, cl / (1.f - c.a));

					c += cl;
				}
			}
		}
	}

	const auto colorSpace = savingFormat->imageDataType == eImageDataType::Byte ? eImageColorSpace::sRGB : eImageColorSpace::Linear;
	const auto processing = savingFormat->wantsStraightAlpha ? eAlphaProcessing::PremultipliedToStraight : eAlphaProcessing::None;

	saveRenderingImage(fileName, buffers.denoisedBuffer ? buffers.denoisedBuffer : buffers.frameBuffer, imageManager, colorSpace, processing);

	if (!extraChannels)
		return;

	if (buffers.denoisedBuffer) {
		auto strOutput = QString("%1/%2-%3.%4").arg(fi.path()).arg(fi.completeBaseName()).arg("original").arg(fi.suffix());
		saveRenderingImage(QDir::toNativeSeparators(strOutput), buffers.frameBuffer, imageManager, colorSpace, processing);
	}

	if (buffers.normalBuffer) {
		auto strOutput = QString("%1/%2-%3.%4").arg(fi.path()).arg(fi.completeBaseName()).arg("normals").arg(fi.suffix());
		saveRenderingImage(QDir::toNativeSeparators(strOutput), buffers.normalBuffer, imageManager);
	}

	if (buffers.depthBuffer) {
		auto strOutput = QString("%1/%2-%3.%4").arg(fi.path()).arg(fi.completeBaseName()).arg("depth").arg(fi.suffix());
		saveRenderingImage(QDir::toNativeSeparators(strOutput), buffers.depthBuffer, imageManager);
	}

	if (buffers.albedoBuffer) {
		auto strOutput = QString("%1/%2-%3.%4").arg(fi.path()).arg(fi.completeBaseName()).arg("albedo").arg(fi.suffix());
		saveRenderingImage(QDir::toNativeSeparators(strOutput), buffers.albedoBuffer, imageManager);
	}

	if (buffers.objectsBuffer) {
		auto strOutput = QString("%1/%2-%3.%4").arg(fi.path()).arg(fi.completeBaseName()).arg("objects").arg(fi.suffix());
		saveRenderingImage(QDir::toNativeSeparators(strOutput), buffers.objectsBuffer, imageManager);
	}

	if (buffers.materialsBuffer) {
		auto strOutput = QString("%1/%2-%3.%4").arg(fi.path()).arg(fi.completeBaseName()).arg("materials").arg(fi.suffix());
		saveRenderingImage(QDir::toNativeSeparators(strOutput), buffers.materialsBuffer, imageManager);
	}

	if (buffers.layerBuffers.size() > 1) {
		for (const auto & buffer : buffers.layerBuffers) {
			if (buffer.second) {
				auto strOutput = QString("%1/%2-%3-%4.%5").arg(fi.path()).arg(fi.completeBaseName()).arg("layer").arg(buffer.first.c_str()).arg(fi.suffix());
				saveRenderingImage(QDir::toNativeSeparators(strOutput), buffer.second, imageManager, colorSpace, processing);
			}
		}
	}
}

void RenderingProxy::save()
{
	QFileInfo fileInfo(m_settings.value(SETTINGS_LAST_SAVED_IMAGE).toString());
	auto defaultSuffix = fileInfo.suffix();
	if (defaultSuffix.isEmpty()) defaultSuffix = "png";

	const auto * imageManager = m_ctx.appContext->imageManager();
	const auto filters = buildFilterString(imageManager->savingFilters());

	// Since we are running in QML window overlay and RenderingProxy has no QWidget,
	// we pass nullptr as parent. QFileDialog handles nullptr gracefully on macOS and Windows.
	QFileDialog saveDialog(nullptr, "Save As", fileInfo.absolutePath(), filters);
	saveDialog.setAcceptMode(QFileDialog::AcceptSave);
	saveDialog.selectFile(fileInfo.absoluteFilePath());
	saveDialog.setDefaultSuffix(defaultSuffix);
	if (!saveDialog.exec())
		return;

	auto fileName = saveDialog.selectedFiles().first();
	if (fileName.isEmpty())
		return;

	m_settings.setValue(SETTINGS_LAST_SAVED_IMAGE, fileName);

	try {
		saveRenderingBuffers(m_renderingBuffers, m_ctx.extraChannels, m_denoised, fileName, imageManager);
		qDebug() << "Saved to" << fileName;
	} catch (const std::exception& ex) {
		qDebug() << "Unable to save" << QFileInfo(fileName).fileName() << ex.what();
	}
}

int RenderingProxy::rendering(const RenderingContext & ctx)
{
	m_denoised = false;

	auto * imageManager = ctx.appContext->imageManager();
	auto * rendererWidget = ctx.core->createRenderer(1.0f, false);
	rendererWidget->enablePreview(false);

	RenderingParameters renderingParams;
	renderingParams.algorithm = ctx.algorithm;
	renderingParams.maxIntensity = ctx.maxIntensity;
	renderingParams.photonScale = ctx.photonRadius;

	const int ITERATIONS_PER_STEP = 10;
	const auto MAX_PROGRESS = ctx.denoise ? 0.95f : 1.f;

	try {
		QElapsedTimer timer;
		timer.start();
		int iterationCount = 0;
		float currentProgress = 0.f;
		emit progress(0.f);

		while (!m_stop) {
			int iterationsToDo = ITERATIONS_PER_STEP;

			if (ctx.numIterations > 0)
				iterationsToDo = std::min(ctx.numIterations - iterationCount, iterationsToDo);

			if (ctx.maxDuration > 0 && ctx.maxDuration - (timer.elapsed() / 1000) < 10)
				iterationsToDo = 1;

			m_totalIterationCount += ctx.core->render(m_renderingBuffers, renderingParams, iterationsToDo, m_totalIterationCount, m_stop);
			iterationCount += iterationsToDo;

			m_imageProvider->setImage(m_renderingBuffers.frameBuffer->toQImage(true));
			emit previewChanged(QString::number(m_totalIterationCount));

			currentProgress = std::min(ctx.maxDuration > 0 ? (timer.elapsed() / 1000 + 1) / (float)ctx.maxDuration : iterationCount / (float)ctx.numIterations, 1.f);
			emit progress(currentProgress * MAX_PROGRESS);

			if (ctx.maxDuration > 0 && timer.elapsed() / 1000 >= ctx.maxDuration)
				break;

			if (ctx.numIterations > 0 && iterationCount >= ctx.numIterations)
				break;
		}

		if (ctx.denoise && m_renderingBuffers.denoisedBuffer) {
			ctx.core->denoise(m_renderingBuffers);
			m_denoised = true;

			m_imageProvider->setImage(m_renderingBuffers.denoisedBuffer->toQImage(true));
			emit previewChanged(QString::number(m_totalIterationCount) + "!");
		}

		if (!m_stop) {
			emit progress(1.f);
		}
	} catch (const std::exception & ex) {
		qDebug() << "Rendering error:" << ex.what();
	}

	emit renderingFinished();

	rendererWidget->enablePreview(true);

	return 0;
}
