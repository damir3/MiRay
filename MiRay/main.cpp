#include "MainWindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include "../Core/Application.h"
#include "ParametersParser.h"
#include "ConsoleRendering.h"

int main(int argc, char *argv[])
{
	qputenv("QML_USE_GLYPHCACHE_WORKAROUND", "1");

	QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

	Application app(argc, argv);

	ParametersParser parser(app.arguments());
	QVariantMap argsMap = parser.parsedArgs();

	if (argsMap.contains("crashdump")) {
		return -1;
	}

	QStringList scenesFilePath;
	for (const auto& v : parser.parsedFilenames()) {
		scenesFilePath << QDir::toNativeSeparators(QFileInfo(v).absoluteFilePath());
	}

	auto strHardwareError = Application::checkHardware();

	if (isConsoleMode(argsMap)) {
#ifdef Q_OS_WIN
		redirectIOToConsole();
#endif
		if (!strHardwareError.isEmpty()) {
			fprintf(stdout, "MiRay can't start because of error: %s\n", strHardwareError.toUtf8().data());
			return 3;
		}

		app.init(eLoggingMode::Console);
		app.startPlugins();

		return processInConsoleMode(argsMap, scenesFilePath);
	}

	if (!strHardwareError.isEmpty()) {
		QMessageBox::critical(nullptr, "MiRay Cannot Start", QString("Hardware check failed with error: %1").arg(strHardwareError));
		return 3;
	}

	app.init(eLoggingMode::File);
	app.startPlugins();

	MainWindow w(app);
	w.show();

	return app.exec();
}
