#pragma once

#include "../Shared/Interfaces/Plugin.h"
class IApplicationContext;

using PluginPtr = std::unique_ptr<IPlugin>;

class CORE_EXPORT PluginManager : public QObject
{
	Q_OBJECT

	std::vector<PluginPtr> m_plugins;

	void getVersionInfo(const QString& fileName, int& major, int& minor, int& build);
	bool loadPlugin(const QString &name, ILog &log);

public:
	PluginManager() {}

	size_t count() const { return m_plugins.size(); }
	IPlugin *get(size_t i) { return m_plugins[i].get(); }

	void loadPlugins(ILog &log);

	void activateInContext(IApplicationContext *context);

signals:
	void started(size_t);
	void finished();
	void loadPlugin(int, const QString&);
	void startPlugin();
};
