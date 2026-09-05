#pragma once

#include "SimpleParams.h"
#include "ParamProxy.h"
#include "RenderingProxy.h"

class QSettings;
class ICoreInstance;
class IApplicationContext;
class QQmlContext;

class RenderingParametersProxy : public QObject
{
	Q_OBJECT

	QSettings & m_settings;
	QQmlContext * m_context;
	ICoreInstance & m_core;
	IApplicationContext & m_appCtx;

	RenderingContext m_options;

	SimpleBooleanParameter m_keepAspectRatioParam;
	BooleanParamProxy m_keepAspectRatio;
	SimpleIntegerParameter m_widthParam;
	IntegerParamProxy m_width;
	SimpleIntegerParameter m_heightParam;
	IntegerParamProxy m_height;
	SimpleEnumParameter m_renderingPresetParam;
	EnumParamProxy m_renderingPreset;
	SimpleIntegerParameter m_numIterationsParam;
	SimpleIntegerParameter m_numIterationsDraftParam;
	SimpleIntegerParameter m_numIterationsProductionParam;
	IntegerParamProxy m_numIterations;
	SimpleScalarParameter m_maxIntensityParam;
	ScalarParamProxy m_maxIntensity;
	SimpleBooleanParameter m_denoiseParam;
	BooleanParamProxy m_denoise;
	SimpleBooleanParameter m_extraChannelsParam;
	BooleanParamProxy m_extraChannels;

	double m_aspect;
	bool m_ignoreSignal;

	QVariantMap m_data;

private slots:
	void onWidthChanged();
	void onHeightChanged();
	void onPresetChanged();

public:
	RenderingParametersProxy(QSettings & settings, QQmlContext * context, ICoreInstance & core, IApplicationContext & appCtx, QWidget * parent);
	~RenderingParametersProxy() override;

	void accept();
	const RenderingContext & options() const { return m_options; }
};
