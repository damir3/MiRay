#pragma once

// MicrofacetDistribution Declarations
class MicrofacetDistribution
{
public:
	// MicrofacetDistribution Public Methods
	virtual ~MicrofacetDistribution() {}

	virtual float d(const vec3 &wh) const = 0;

	virtual float lambda(const vec3 &w) const = 0;

	float g1(const vec3 &w) const
	{
//		if (Dot(w, wh) * cosTheta(w) < 0.) return 0.;
		return 1.f / (1.f + lambda(w));
	}

	float g(const vec3 &wo, const vec3 &wi) const
	{
		return 1.f / (1.f + lambda(wo) + lambda(wi));
	}

	virtual vec3 sample_wh(const vec3 &wo, const vec2 &u) const = 0;

	float pdf(const vec3 &wo, const vec3 &wh) const;

protected:
	// MicrofacetDistribution Protected Methods
	MicrofacetDistribution(bool sampleVisibleArea) : m_sampleVisibleArea(sampleVisibleArea) {}

	// MicrofacetDistribution Protected Data
	const bool m_sampleVisibleArea;
};

class BeckmannDistribution final : public MicrofacetDistribution
{
public:
	// BeckmannDistribution Public Methods
//	static float RoughnessToAlpha(float roughness)
//	{
//		roughness = std::max(roughness, 1e-3f);
//		float x = std::log(roughness), x2 = x * x, x3 = x2 * x, x4 = x3 * x;
//		return 1.62142f + 0.819955f * x + 0.1734f * x2 + 0.0171201f * x3 + 0.000640711f * x4;
//	}

	BeckmannDistribution(const vec2 & alpha, bool samplevis = true)
		: MicrofacetDistribution(samplevis), m_alpha(alpha) {}

	float d(const vec3 &wh) const;

	vec3 sample_wh(const vec3 &wo, const vec2 &u) const;

private:
	// BeckmannDistribution Private Methods
	float lambda(const vec3 &w) const;

	// BeckmannDistribution Private Data
	const vec2 m_alpha;
};

class TrowbridgeReitzDistribution final : public MicrofacetDistribution
{
public:
	// TrowbridgeReitzDistribution Public Methods
//	static inline float RoughnessToAlpha(float roughness)
//	{
//		roughness = std::max(roughness, 1e-3f);
//		float x = std::log(roughness), x2 = x * x, x3 = x2 * x, x4 = x3 * x;
//		return 1.62142f + 0.819955f * x + 0.1734f * x2 + 0.0171201f * x3 + 0.000640711f * x4;
//	}

	TrowbridgeReitzDistribution(const vec2 & alpha, bool samplevis = true)
		: MicrofacetDistribution(samplevis), m_alpha(alpha) {}

	float d(const vec3 &wh) const;

	vec3 sample_wh(const vec3 &wo, const vec2 &u) const;

private:
	// TrowbridgeReitzDistribution Private Methods
	float lambda(const vec3 &w) const;

	// TrowbridgeReitzDistribution Private Data
	const vec2 m_alpha;
};
