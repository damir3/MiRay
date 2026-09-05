#pragma once

#include "../Shared/Interfaces/Plugin.h"

class FormatModels3DPlugin : public IPlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.miray.FormatModels3D")
	Q_INTERFACES(IPlugin)

public:
	FormatModels3DPlugin();
	~FormatModels3DPlugin();

	void activateInContext(IApplicationContext *appContext) override;
};
