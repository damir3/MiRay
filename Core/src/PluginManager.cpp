#include "../PluginManager.h"

#ifdef WIN32
	#pragma comment(lib, "Version.lib")
	#include "Windows.h"
#endif

void PluginManager::loadPlugins(ILog &log)
{
	QDir appPath = qApp->applicationDirPath();

#ifdef __APPLE__
	if (appPath.dirName() == "MacOS")
		appPath.cd("../Frameworks");
#endif

	QStringList dlls;
	for (auto fileName : appPath.entryList(QDir::Files)) {
		if (fileName.endsWith(".plugin", Qt::CaseInsensitive))
			dlls.push_back(appPath.absoluteFilePath(fileName));
	}

	log.information("Loading plugins");

	emit started(dlls.size());
	int i = 0;
	QFileInfo fileName;
	for (auto dll : dlls) {
		fileName.setFile(dll);
		emit loadPlugin(++i, fileName.fileName());
		loadPlugin(dll, log);
	}
	emit finished();

	log.information("Plugins loaded");
}

void PluginManager::getVersionInfo(const QString& fileName, int& major, int& minor, int& build)
{
#ifdef Q_OS_WIN
	QString fName = nativePath(fileName);

	LPCTSTR nativeFileName = (LPCWSTR)fName.utf16();
	DWORD  verSize = ::GetFileVersionInfoSize(nativeFileName, nullptr);
	if (verSize) {
		LPSTR verData = new char[verSize];
		if (::GetFileVersionInfo(nativeFileName, 0, verSize, verData)) {
			UINT size = 0;
			VS_FIXEDFILEINFO* verInfo = nullptr;
			if (VerQueryValue(verData, L"\\", reinterpret_cast<LPVOID*>(&verInfo), &size)) {
				if (size) {
					major = HIWORD(verInfo->dwFileVersionMS);
					minor = LOWORD(verInfo->dwFileVersionMS);
					build = HIWORD(verInfo->dwFileVersionLS);
				}
			}
		}
		delete[] verData;
	}
#elif defined Q_OS_MAC
	major = VER_MAJOR;
	minor = VER_MINOR;
	build = VER_BUILD;
#endif
}

bool PluginManager::loadPlugin(const QString &name, ILog &log)
{
	QString pluginName = QFileInfo(name).fileName();
	log.information(QString("Loading '%1'").arg(pluginName));

	int major = 0, minor = 0, build = 0;
	getVersionInfo(name, major, minor, build);
	if (major != VER_MAJOR || minor != VER_MINOR || build != VER_BUILD) {
		log.warning(QString("Plugin has incompatible version %1.%2.%3, skipped").arg(major).arg(minor).arg(build));
		return false;
	}

	QPluginLoader loader(name);
	if (!loader.load()) {
		log.warning(QString("'%1' is not a plugin, skipped. %2").arg(pluginName).arg(loader.errorString()));
		return false;
	}

	QObject *instance = loader.instance();
	IPlugin *plugin = qobject_cast<IPlugin *>(instance);

	if (plugin) {
		m_plugins.push_back(PluginPtr(plugin));
	} else {
		log.warning(QString("'%1' is not a plugin, skipped").arg(pluginName));
		delete instance;
	}

	return plugin ? true : false;
}

void PluginManager::activateInContext(IApplicationContext *context)
{
	emit startPlugin();
	for (size_t i = 0; i < m_plugins.size(); i++)
		m_plugins[i]->activateInContext(context);
}
