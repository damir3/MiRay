#include "ConsoleRendering.h"

#include "../Shared/Interfaces/Log.h"
#include "../Shared/Interfaces/Scene.h"
#include "../Shared/Interfaces/SerializationContext.h"
#include "../Shared/Interfaces/Camera.h"
#include "../Shared/Interfaces/Image.h"
#include "../Shared/Interfaces/Material.h"
#include "../Shared/Interfaces/MaterialManager.h"
#include "../Shared/Utils/FileUtils.h"

#include "BatchRenderingParametersHelper.h"
#include "RenderingProxy.h"

#include <QFileInfo>
#include <QDateTime>
#include <QDir>

#ifdef Q_OS_WIN
#include <windows.h>
#include <stdio.h>

void redirectIOToConsole()
{
	if (AttachConsole(ATTACH_PARENT_PROCESS)) {
		FILE *fo = nullptr, *fe = nullptr;
		freopen_s(&fo, "CONOUT$", "w", stdout);
		freopen_s(&fe, "CONOUT$", "w", stdout);
	}
}
#endif

constexpr int PREVIEW_WIDTH = 256;
constexpr int PREVIEW_HEIGHT = 256;
constexpr int PREVIEW_PASSES = 100;
constexpr float MAX_RENDERING_INTENSITY = 100.f;

int consoleRendering(QVariantMap params, QStringList scenes);

bool isConsoleMode(const QVariantMap& args)
{
	if (args.contains("render")) return true;
	if (args.contains("verify")) return true;
	if (args.contains("thumbnail")) return true;
	if (args.contains("convert")) return true;
	return false;
}

int processInConsoleMode(const QVariantMap& args, const QStringList& scenes)
{
	QVariantMap params;

	for (auto it = args.begin(); it != args.end(); it++)
		params[it.key().toLower()] = it.value().toString();

	return consoleRendering(params, scenes);
}

//////////////////////////////////////////////////////////////////////////

void printUsage()
{
	fprintf(stdout,
		"\n"
		"Usage:\n"
		"	MiRay --verify scene.mirayScene [scene2.mirayScene ...]\n"
		"	MiRay --render [--width=N] [--height=N] [--passes=N] [--algorithm=<VPT|UPBP>] [--output-ext=<extension>] [--output=<filename>] [--extra-channels] [--denoise] scene.mirayScene [scene2.mirayScene ...]\n"
		"	MiRay --thumbnail [--passes=N] material.mirayMaterial scene.mirayScene\n"
		"	MiRay --convert scene-in-some-format.xxx scene.mirayScene\n"
		"\n"
	);
}

int verifyScene(QString scenePath)
{
	auto app = static_cast<Application *>(qApp);
	auto log = app->log();

	log->information(QString("Verifying scene %1...").arg(QFileInfo(scenePath).completeBaseName()));

	std::shared_ptr<ICoreInstance> core(app->createCoreInstance());

	try {
		if (!core->loadFile(scenePath, LoadingParameters::forSceneLoading()))
			throw std::runtime_error("Unable to load file.");
	} catch (const std::exception &ex) {
		log->error(ex.what());
		return 1;
	}

	return 0;
}

int convertScene(QString from, QString to)
{
	auto app = static_cast<Application *>(qApp);
	auto log = app->log();

	log->information(QString("Converting scene '%1' to '%2'...").arg(QFileInfo(from).fileName()).arg(QFileInfo(to).fileName()));

	std::shared_ptr<ICoreInstance> core(app->createCoreInstance());

	try {
		if (!core->loadFile(from, LoadingParameters::forSceneLoading()))
			throw std::runtime_error("Unable to load file.");

		core->saveFile(to, SavingParameters::forSceneSaving());
	} catch (const std::exception &ex) {
		log->error(ex.what());
		return 1;
	}

	return 0;
}

int renderScene(QString scenePath, QVariantMap params)
{
	auto app = static_cast<Application *>(qApp);
	auto log = app->log();
	auto imageManager = app->imageManager();

	log->information(QString("Rendering scene %1...").arg(QFileInfo(scenePath).completeBaseName()));

	std::shared_ptr<ICoreInstance> core(app->createCoreInstance());

	try {
		if (!core->loadFile(scenePath, LoadingParameters::forSceneLoading()))
			throw std::runtime_error("Unable to load file.");

		QString outputPath;
		if (params.contains("output")) {
			outputPath = params["output"].toString();
			log->information(QString(" Overriding output image to: %1").arg(outputPath));
		} else {
			QString ext = "png";
			if (params.contains("output-ext")) {
				ext = params.value("output-ext", "png").toString();
				log->information(QString(" Overriding output extension to: %1").arg(ext));
			}
			outputPath = QString("%1.%2").arg(scenePath).arg(ext);
			log->information(QString(" Rendered image will be saved to: %1").arg(outputPath));
		}

		auto scene = core->getScene();
		auto &camera = scene->camera();
		float aspect = std::clamp(camera.aspect().get(), 0.1f, 10.f);

		BatchRenderingParametersHelper helper(scene->metadata(), params, aspect);

		const RenderingParameters &rp = helper.params();

		int imageWidth = rp.renderArea ? rp.renderArea->tileSize.x : helper.width();
		int imageHeight = rp.renderArea ? rp.renderArea->tileSize.y : helper.height();

		RenderingBuffers buffers;
		uint32_t types = helper.renderExtraChannels() ? -1 : 0;
		if (helper.denoise())
			types |= BufferType_Denoised;
		else
			types &= ~BufferType_Denoised;
		core->createRenderingBuffers(buffers, imageWidth, imageHeight, types);
		if (helper.renderExtraChannels())
			log->information(" Will be rendering extra channels (depth, normals, etc)");

		int currentIteration = 0;
		QDateTime dtStart = QDateTime::currentDateTimeUtc();

		bool useTime = !helper.usePasses();
		int iterations = helper.passes();
		int seconds = helper.seconds();

		bool stop = false;

		while (true) {
			double progress;
			int iterationsToGo = 10;

			if (useTime) {
				QDateTime dtNow = QDateTime::currentDateTimeUtc();
				int secs = static_cast<int>(dtStart.secsTo(dtNow));
				if (secs >= seconds)
					break;

				progress = static_cast<double>(secs) / seconds;

				log->information(QString(" - Rendering %1 of %2 seconds").arg(secs).arg(seconds));
			} else {
				if (currentIteration >= iterations)
					break;

				progress = static_cast<double>(currentIteration) / iterations;
				int remaining = iterations - currentIteration;
				if (remaining < iterationsToGo)
					iterationsToGo = remaining;

				log->information(QString(" - Rendering passes %1..%2 of %3").arg(currentIteration).arg(currentIteration + iterationsToGo).arg(iterations));
			}

			int iterationsDone = core->render(buffers, helper.params(), iterationsToGo, currentIteration, stop);
			currentIteration += iterationsDone;
		}

		if (helper.denoise()) {
			log->information(QString(" Reducing noise"));
			core->denoise(buffers);
		}

		log->information(QString(" Rendering is done, saving image"));

		saveRenderingBuffers(buffers, helper.renderExtraChannels(), helper.denoise(), outputPath, imageManager);

		log->information(QString(" Saved to '%1'").arg(outputPath));

	} catch (const std::exception &ex) {
		log->error(ex.what());
		return 1;
	}

	return 0;
}

int updateThumbnail(QString material, QString scene, int passes = PREVIEW_PASSES * 2)
{
	auto app = static_cast<Application *>(qApp);
	auto log = app->log();

	if (!QFileInfo(material).exists()) {
		log->error(QString("Unable to find material file '%1'").arg(material));
		return 1;
	}

	if (!QFileInfo(scene).exists()) {
		log->error(QString("Unable to find scene file '%1'").arg(scene));
		return 1;
	}

	auto mtlData = readFile(material);
	ModelLoadingContext lctx;
	ModelSavingContext sctx;

	auto imageManager = app->imageManager();

	log->information(QString("Updating material '%1' with scene '%2'...").arg(QFileInfo(material).completeBaseName()).arg(QFileInfo(scene).completeBaseName()));

	std::shared_ptr<ICoreInstance> core(app->createCoreInstance());

	try {
		if (!core->loadFile(scene, LoadingParameters::forSceneLoading()))
			throw std::runtime_error("Unable to load file.");

		const int MAX_PASSES = passes > 0 ? passes : PREVIEW_PASSES * 2;
		RenderingParameters rp;
		rp.algorithm = RenderingAlgorithm_VolumePathTracing;
		rp.maxIntensity = MAX_RENDERING_INTENSITY;

		auto sceneObj = core->getScene();
		auto & materialManager = sceneObj->materialManager();

		auto oldMaterial = materialManager.getByName("Material");
		if (!oldMaterial)
			throw std::logic_error("Unable to find material named 'Material' in the scene");

		auto newMaterial = materialManager.load(mtlData, lctx, materialManager.count());
		sceneObj->replaceMaterial(oldMaterial, newMaterial);

		RenderingBuffers buffers;
		core->createRenderingBuffers(buffers, PREVIEW_WIDTH, PREVIEW_HEIGHT, BufferType_Denoised);

		const int STEP = 10;
		bool stop = false;

		for (int i = 0; i < MAX_PASSES; i += STEP) {
			int passesToRender = std::min(MAX_PASSES - i, STEP);
			log->information(QString(" - Rendering passes %1..%2 of %3").arg(i + 1).arg(i + passesToRender).arg(MAX_PASSES));

			core->render(buffers, rp, passesToRender, i, stop);
		}
		log->information(QString(" Rendering done, saving image"));

		core->denoise(buffers);
		newMaterial->setPreview(buffers.denoisedBuffer->toQImage(true));

		log->information(QString(" Serializing material"));
		auto mtlDataNew = newMaterial->save(sctx);

		log->information(QString(" Replacing old material with the new one"));
		writeFile(material, mtlDataNew);

		log->information(QString(" All done"));
	} catch (const std::exception &ex) {
		log->error(ex.what());
		return 1;
	}

	return 0;
}

int consoleRendering(QVariantMap params, QStringList scenes)
{
	if (scenes.empty()) {
		printUsage();
		return 1;
	}

	bool isVerify = params.contains("verify");
	bool isRender = params.contains("render");
	bool isThumbnail = params.contains("thumbnail");
	bool isConvert = params.contains("convert");

	int cnt = 0;
	if (isVerify) cnt++;
	if (isRender) cnt++;
	if (isThumbnail) cnt++;
	if (isConvert) cnt++;

	if (cnt != 1) {
		printUsage();
		return 1;
	}

	if (isThumbnail) {
		if (scenes.length() != 2) {
			printUsage();
			return 1;
		}

		auto mtl = scenes[0];
		auto scene = scenes[1];

		QString mtlSuffix = QFileInfo(mtl).suffix();
		QString sceneSuffix = QFileInfo(scene).suffix();
		if ((mtlSuffix.compare("mirayMaterial", Qt::CaseInsensitive) != 0 && mtlSuffix.compare("owletMaterial", Qt::CaseInsensitive) != 0) ||
			(sceneSuffix.compare("mirayScene", Qt::CaseInsensitive) != 0 && sceneSuffix.compare("owletScene", Qt::CaseInsensitive) != 0)) {
			printUsage();
			return 1;
		}

		int passes = params.value("passes", PREVIEW_PASSES * 2).toInt();
		return updateThumbnail(mtl, scene, passes);
	}

	if (isConvert) {
		if (scenes.length() != 2) {
			printUsage();
			return 1;
		}

		auto from = scenes[0];
		auto to = scenes[1];

		bool inputValid = QFileInfo(from).exists();
		bool outputValid = QFileInfo(to).suffix().compare("mirayScene", Qt::CaseInsensitive) == 0 || QFileInfo(to).suffix().compare("owletScene", Qt::CaseInsensitive) == 0;

		if (!inputValid || !outputValid) {
			printUsage();
			return 1;
		}

		return convertScene(from, to);
	}

	for (auto scene : scenes) {
		int res;
		if (isVerify)
			res = verifyScene(scene);
		else
			res = renderScene(scene, params);

		if (res)
			return res;
	}

	return 0;
}
