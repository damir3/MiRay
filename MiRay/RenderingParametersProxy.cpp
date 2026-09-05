#include "RenderingParametersProxy.h"

#include "../Shared/Interfaces/Camera.h"
#include "../Shared/Interfaces/Scene.h"
#include <QSettings>
#include <QQmlContext>
#include <cmath>

static const QString SETTINGS_RENDERING_OPTIONS = "renderingOptions";
static const QString SETTINGS_WIDTH = "width";
static const QString SETTINGS_HEIGHT = "height";
static const QString SETTINGS_KEEP_ASPECT_RATIO = "keepAspectRatio";
static const QString SETTINGS_RENDERING_PRESET = "renderingPreset";
static const QString SETTINGS_NUM_ITERATIONS = "numIterations";
static const QString SETTINGS_MAX_INTENSITY = "maxIntensity";
static const QString SETTINGS_DENOISE = "denoise";
static const QString SETTINGS_EXTRA_CHANNELS = "extraChannels";

RenderingParametersProxy::RenderingParametersProxy(QSettings & settings, QQmlContext * context, ICoreInstance & core, IApplicationContext & appCtx, QWidget * parent)
	: m_settings(settings)
	, m_context(context)
	, m_core(core)
	, m_appCtx(appCtx)
	, m_keepAspectRatioParam(true)
	, m_keepAspectRatio("Keep aspect ratio")
	, m_widthParam(1024, 1, 8192)
	, m_width("Width")
	, m_heightParam(768, 1, 8192)
	, m_height("Height")
	, m_renderingPresetParam(0, QString("Draft,Production,Iteration-based,Time-based").split(","))
	, m_renderingPreset("Rendering preset")
	, m_numIterationsParam(2000, 1, 1000000)
	, m_numIterationsDraftParam(100, 1, 1000)
	, m_numIterationsProductionParam(2000, 1, 1000000)
	, m_numIterations("Number of iterations")
	, m_maxIntensityParam(100.f, 1.f, 10000.f, 1)
	, m_maxIntensity("Brightness limit")
	, m_denoiseParam(true)
	, m_denoise("Reduce noise")
	, m_extraChannelsParam(false)
	, m_extraChannels("Render extra channels")
	, m_aspect(1.0)
	, m_ignoreSignal(false)
{
	m_settings.beginGroup(SETTINGS_RENDERING_OPTIONS);
	m_widthParam._set(m_settings.value(SETTINGS_WIDTH, m_widthParam.get()).toInt());
	m_heightParam._set(m_settings.value(SETTINGS_HEIGHT, m_heightParam.get()).toInt());
	m_keepAspectRatioParam._set(m_settings.value(SETTINGS_KEEP_ASPECT_RATIO, m_keepAspectRatioParam.get()).toBool());
	m_renderingPresetParam._setIndex(m_settings.value(SETTINGS_RENDERING_PRESET, m_renderingPresetParam.getIndex()).toInt());
	m_numIterationsParam._set(m_settings.value(SETTINGS_NUM_ITERATIONS, m_numIterationsParam.get()).toInt());
	m_maxIntensityParam._set(m_settings.value(SETTINGS_MAX_INTENSITY, m_maxIntensityParam.get()).toFloat());
	m_denoiseParam._set(m_settings.value(SETTINGS_DENOISE, m_denoiseParam.get()).toBool());
	m_extraChannelsParam._set(m_settings.value(SETTINGS_EXTRA_CHANNELS, m_extraChannelsParam.get()).toBool());
	m_settings.endGroup();

	m_aspect = (double)m_widthParam.get() / (double)m_heightParam.get();

	m_numIterationsDraftParam.setEnabled(false);
	m_numIterationsProductionParam.setEnabled(false);

	m_keepAspectRatio.setParam(&m_keepAspectRatioParam);
	m_width.setParam(&m_widthParam);
	m_height.setParam(&m_heightParam);
	m_renderingPreset.setParam(&m_renderingPresetParam);
	m_maxIntensity.setParam(&m_maxIntensityParam);
	m_denoise.setParam(&m_denoiseParam);
	m_extraChannels.setParam(&m_extraChannelsParam);

	onPresetChanged();

	connect(&m_widthParam, SIGNAL(changed()), this, SLOT(onWidthChanged()));
	connect(&m_heightParam, SIGNAL(changed()), this, SLOT(onHeightChanged()));
	connect(&m_renderingPresetParam, SIGNAL(changed()), this, SLOT(onPresetChanged()));

	m_data["keepAspectRatio"] = QVariant::fromValue(static_cast<QObject *>(&m_keepAspectRatio));
	m_data["width"] = QVariant::fromValue(static_cast<QObject *>(&m_width));
	m_data["height"] = QVariant::fromValue(static_cast<QObject *>(&m_height));
	m_data["renderingPreset"] = QVariant::fromValue(static_cast<QObject *>(&m_renderingPreset));
	m_data["numIterations"] = QVariant::fromValue(static_cast<QObject *>(&m_numIterations));
	m_data["maxIntensity"] = QVariant::fromValue(static_cast<QObject *>(&m_maxIntensity));
	m_data["denoise"] = QVariant::fromValue(static_cast<QObject *>(&m_denoise));
	m_data["extraChannels"] = QVariant::fromValue(static_cast<QObject *>(&m_extraChannels));

	m_options.appContext = &m_appCtx;
	m_options.core = &m_core;
	if (auto scene = m_core.getScene()) {
		m_aspect = scene->camera().aspect().get();
		if (m_keepAspectRatioParam.get())
			m_widthParam.set((int)std::round(m_heightParam.get() * m_aspect));
	}

	m_context->setContextProperty("renderingParametersModel", m_data);
}

RenderingParametersProxy::~RenderingParametersProxy()
{
	m_context->setContextProperty("renderingParametersModel", nullptr);
}

void RenderingParametersProxy::onWidthChanged()
{
	if (!m_ignoreSignal) {
		m_ignoreSignal = true;
		if (m_keepAspectRatioParam.get())
			m_heightParam.set((int)std::round(m_widthParam.get() / m_aspect));
		else
			m_aspect = (double)m_widthParam.get() / (double)m_heightParam.get();
		m_ignoreSignal = false;
	}
}

void RenderingParametersProxy::onHeightChanged()
{
	if (!m_ignoreSignal) {
		m_ignoreSignal = true;
		if (m_keepAspectRatioParam.get())
			m_widthParam.set((int)std::round(m_heightParam.get() * m_aspect));
		else
			m_aspect = (double)m_widthParam.get() / (double)m_heightParam.get();
		m_ignoreSignal = false;
	}
}

void RenderingParametersProxy::onPresetChanged()
{
	switch (m_renderingPresetParam.getIndex()) {
		case 0: m_numIterations.setParam(&m_numIterationsDraftParam); break; // draft
		case 1: m_numIterations.setParam(&m_numIterationsProductionParam); break; // production
		case 2: m_numIterations.setParam(&m_numIterationsParam); break; // iteration-based
		default: break;
	}
}

void RenderingParametersProxy::accept()
{
	m_settings.beginGroup(SETTINGS_RENDERING_OPTIONS);
	m_settings.setValue(SETTINGS_WIDTH, m_widthParam.get());
	m_settings.setValue(SETTINGS_HEIGHT, m_heightParam.get());
	m_settings.setValue(SETTINGS_KEEP_ASPECT_RATIO, m_keepAspectRatioParam.get());
	m_settings.setValue(SETTINGS_RENDERING_PRESET, m_renderingPresetParam.getIndex());
	m_settings.setValue(SETTINGS_NUM_ITERATIONS, m_numIterationsParam.get());
	m_settings.setValue(SETTINGS_MAX_INTENSITY, m_maxIntensityParam.get());
	m_settings.setValue(SETTINGS_DENOISE, m_denoiseParam.get());
	m_settings.setValue(SETTINGS_EXTRA_CHANNELS, m_extraChannelsParam.get());
	m_settings.endGroup();

	m_options.width = m_widthParam.get();
	m_options.height = m_heightParam.get();
	m_options.algorithm = RenderingAlgorithm_VolumePathTracing;
	m_options.maxDuration = 0;
	switch (m_renderingPresetParam.getIndex()) {
		case 0: m_options.numIterations = m_numIterationsDraftParam.get(); break; // draft
		case 1: m_options.numIterations = m_numIterationsProductionParam.get(); break; // production
		case 2: m_options.numIterations = m_numIterationsParam.get(); break; // iteration-based
		default: m_options.numIterations = m_numIterationsParam.get(); break;
	}
	m_options.denoise = m_denoiseParam.get();
	m_options.maxIntensity = m_maxIntensityParam.get();
	m_options.extraChannels = m_extraChannelsParam.get();
}
