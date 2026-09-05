#pragma once

#include "ParamProxy.h"
#include "SimpleParams.h"
#include "../Shared/Interfaces/Settings.h"

class AppSettingsProxy
{
	ISettings& m_settings;
	QQmlContext * const m_context;

	SimpleBooleanParameter m_putLoadedOnTheFloor;
	SimpleBooleanParameter m_previewDenoise;
	SimpleIntegerParameter m_previewFrames;
	SimpleBooleanParameter m_includingBetas;
	SimpleBooleanParameter m_checkUpdates;
	SimpleStringParameter  m_queuedJobsPath;
#ifdef Q_OS_WIN
	SimpleEnumParameter    m_opengl;
#endif

	BooleanParamProxy m_putLoadedOnTheFloorProxy;
	BooleanParamProxy m_previewDenoiseProxy;
	IntegerParamProxy m_previewFramesProxy;
	BooleanParamProxy m_includingBetasProxy;
	BooleanParamProxy m_checkUpdatesProxy;
	StringParamProxy  m_queuedJobsPathProxy;
#ifdef Q_OS_WIN
	EnumParamProxy    m_openglProxy;
#endif

	QVariantMap m_data;

public:
	AppSettingsProxy(ISettings& settings, QQmlContext * context);
	~AppSettingsProxy();

	void accept();
};
