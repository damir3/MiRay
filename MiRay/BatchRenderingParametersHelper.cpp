#include "BatchRenderingParametersHelper.h"

#include "../Shared/Interfaces/Scene.h"

BatchRenderingParametersHelper::BatchRenderingParametersHelper(const QJsonObject &metadata, const QVariantMap &params, float cameraAspect)
	: m_width(0)
	, m_height(0)
	, m_passes(0)
	, m_passesArePasses(true)
	, m_renderExtraChannels(SCENE_METADATA_DEFAULT_RENDERING_EXTRA_CHANNELS)
	, m_denoise(SCENE_METADATA_KEY_RENDERING_DENOISE)
{
	initWithMetadata(metadata);
	initWithCommandLineParams(params);
	checkEssentialParameters(cameraAspect);
	completeRenderAreaIfAny();
}

//////////////////////////////////////////////////////////////////////////

void BatchRenderingParametersHelper::overrideMaxIntensity(float maxIntensity)
{
	m_rp.maxIntensity = maxIntensity;
}

int BatchRenderingParametersHelper::width() const
{
	return m_width;
}

int BatchRenderingParametersHelper::height() const
{
	return m_height;
}

bool BatchRenderingParametersHelper::usePasses() const
{
	return m_passesArePasses;
}

int BatchRenderingParametersHelper::passes() const
{
	return m_passes;
}

int BatchRenderingParametersHelper::seconds() const
{
	return m_passes;
}

bool BatchRenderingParametersHelper::renderExtraChannels() const
{
	return m_renderExtraChannels;
}

bool BatchRenderingParametersHelper::denoise() const
{
	return m_denoise;
}

const RenderingParameters &BatchRenderingParametersHelper::params() const
{
	return m_rp;
}

//////////////////////////////////////////////////////////////////////////

void BatchRenderingParametersHelper::initWithMetadata(const QJsonObject &metadata)
{
	m_width = metadata.value(SCENE_METADATA_KEY_RENDERING_WIDTH).toInt(m_width);
	m_height = metadata.value(SCENE_METADATA_KEY_RENDERING_HEIGHT).toInt(m_height);

	bool advanced = metadata.value(SCENE_METADATA_KEY_RENDERING_ADVANCED).toBool(false);
	if (advanced) {
		m_rp.algorithm = metadata.value(SCENE_METADATA_KEY_RENDERING_CAUSTICS).toBool(false) ? RenderingAlgorithm_UPBP : RenderingAlgorithm_VolumePathTracing;
		m_rp.maxIntensity = metadata.value(SCENE_METADATA_KEY_RENDERING_MAX_INTENSITY).toDouble(SCENE_METADATA_DEFAULT_RENDERING_MAX_INTENSITY_FULL);
		m_rp.photonScale = metadata.value(SCENE_METADATA_KEY_RENDERING_PHOTON_SCALE).toDouble(SCENE_METADATA_DEFAULT_RENDERING_PHOTON_SCALE);
	}

	QString preset = metadata.value(SCENE_METADATA_KEY_RENDERING_PRESET).toString("draft");
	int presetData = metadata.value(SCENE_METADATA_KEY_RENDERING_PRESET_DATA).toInt(m_passes);

	if (preset.compare("draft", Qt::CaseInsensitive) == 0) {
		m_passes = 100;
	} else if (preset.compare("production", Qt::CaseInsensitive) == 0) {
		m_passes = 2000;
	} else if (preset.compare("iterations", Qt::CaseInsensitive) == 0) {
		m_passes = presetData;
	} else if (preset.compare("time", Qt::CaseInsensitive) == 0) {
		m_passes = presetData;
		m_passesArePasses = false;
	}

	m_renderExtraChannels = metadata.value(SCENE_METADATA_KEY_RENDERING_EXTRA_CHANNELS).toBool(SCENE_METADATA_DEFAULT_RENDERING_EXTRA_CHANNELS);
	m_denoise = metadata.value(SCENE_METADATA_KEY_RENDERING_DENOISE).toBool(SCENE_METADATA_DEFAULT_RENDERING_DENOISE);
}

void BatchRenderingParametersHelper::initWithCommandLineParams(const QVariantMap &params)
{
	if (params.contains("width") || params.contains("height")) {
		// override the old values
		m_width = params.value("width", 0).toInt();
		m_height = params.value("height", 0).toInt();
	}
	m_rp.maxIntensity = params.value("max-intensity", m_rp.maxIntensity).toFloat();
	m_rp.photonScale = params.value("photon-scale", m_rp.photonScale).toFloat();

	if (params.contains("passes")) {
		m_passes = params["passes"].toInt();
		m_passesArePasses = true;
	}

	if (params.contains("seconds")) {
		m_passes = params["seconds"].toInt();
		m_passesArePasses = false;
	}

	if (params.contains("algorithm")) {
		auto value = params["algorithm"].toString();
		if (value.compare("UPBP", Qt::CaseInsensitive) == 0)
			m_rp.algorithm = RenderingAlgorithm_UPBP;
		else if (value.compare("VPT", Qt::CaseInsensitive) == 0)
			m_rp.algorithm = RenderingAlgorithm_VolumePathTracing;
		else
			throw std::runtime_error(QString("Unsupported algorithm '%1', use VPT or UPBP instead.").arg(value).toStdString());
	}

	if (params.contains("extra-channels")) {
		m_renderExtraChannels = true;
	}

	if (params.contains("denoise")) {
		m_denoise = true;
	}

	if (params.contains("tile")) {
		static const char *tile_error = "--tile parameter must have 4 integers separated by commas: left,top,width,height. Example: --tile=100,100,200,200";

		auto tileStr = params["tile"].toString();
		auto items = tileStr.split(',');
		if (items.size() != 4)
			throw std::runtime_error(tile_error);

		bool bl, bt, bw, bh;
		int tl = items[0].toInt(&bl);
		int tt = items[1].toInt(&bt);
		int tw = items[2].toInt(&bw);
		int th = items[3].toInt(&bh);

		if (!bl || !bt || !bw || !bh)
			throw std::runtime_error(tile_error);

		if (tw < 0 || th < 0 || tl < 0 || tt < 0)
			throw std::runtime_error(tile_error);

		m_area.tilePos.x = tl;
		m_area.tilePos.y = tt;
		m_area.tileSize.x = tw;
		m_area.tileSize.y = th;

		m_rp.renderArea = &m_area;
	}
}

void BatchRenderingParametersHelper::checkEssentialParameters(float cameraAspect)
{
	if (m_width == 0 && m_height == 0)
		m_width = SCENE_METADATA_DEFAULT_RENDERING_WIDTH;
	if (m_width == 0)
		m_width = static_cast<int>(0.5f + m_height * cameraAspect);
	if (m_height == 0)
		m_height = static_cast<int>(0.5f + m_width / cameraAspect);
	if (m_passes == 0)
		m_passes = 20;
}

void BatchRenderingParametersHelper::completeRenderAreaIfAny()
{
	if (!m_rp.renderArea)
		return;

	m_area.frameSize.x = m_width;
	m_area.frameSize.y = m_height;

	assert(m_area.tilePos.x >= 0);
	assert(m_area.tilePos.y >= 0);
	assert(m_area.tileSize.x >= 0);
	assert(m_area.tileSize.y >= 0);

	if (m_area.tilePos.x >= m_area.frameSize.x || m_area.tilePos.x + m_area.tileSize.x > m_area.frameSize.x || m_area.tilePos.y >= m_area.frameSize.y || m_area.tilePos.y + m_area.tileSize.y > m_area.frameSize.y)
		throw std::runtime_error("Tile goes outside the main image bounds");
}
