#pragma once

#include "../Shared/Interfaces/CoreInstance.h"

class BatchRenderingParametersHelper
{
	int	m_width;
	int m_height;
	RenderingParameters m_rp;
	RenderArea m_area;
	int	m_passes;
	bool m_passesArePasses;
	bool m_renderExtraChannels;
	bool m_denoise;

	void initWithMetadata(const QJsonObject &metadata);
	void initWithCommandLineParams(const QVariantMap &params);
	void checkEssentialParameters(float cameraAspect);
	void completeRenderAreaIfAny();

public:
	BatchRenderingParametersHelper(const QJsonObject &metadata, const QVariantMap &params, float cameraAspect);

	void overrideMaxIntensity(float maxIntensity);

	int width() const;
	int height() const;

	bool usePasses() const;
	int passes() const;
	int seconds() const;

	bool renderExtraChannels() const;
	bool denoise() const;

	const RenderingParameters &params() const;
};
