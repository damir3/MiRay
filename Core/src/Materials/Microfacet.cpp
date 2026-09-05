#include "Microfacet.h"

inline float erfInv(float x)
{
	float w, p;
	x = std::clamp(x, -0.99999f, 0.99999f);
	w = -std::log((1.f - x) * (1.f + x));
	if (w < 5.f) {
		w = w - 2.5f;
		p = 2.81022636e-08f;
		p = 3.43273939e-07f + p * w;
		p = -3.5233877e-06f + p * w;
		p = -4.39150654e-06f + p * w;
		p = 0.00021858087f + p * w;
		p = -0.00125372503f + p * w;
		p = -0.00417768164f + p * w;
		p = 0.246640727f + p * w;
		p = 1.50140941f + p * w;
	} else {
		w = std::sqrt(w) - 3.f;
		p = -0.000200214257f;
		p = 0.000100950558f + p * w;
		p = 0.00134934322f + p * w;
		p = -0.00367342844f + p * w;
		p = 0.00573950773f + p * w;
		p = -0.0076224613f + p * w;
		p = 0.00943887047f + p * w;
		p = 1.00167406f + p * w;
		p = 2.83297682f + p * w;
	}
	return p * x;
}

inline float Erf(float x)
{
	// constants
	float a1 = 0.254829592f;
	float a2 = -0.284496736f;
	float a3 = 1.421413741f;
	float a4 = -1.453152027f;
	float a5 = 1.061405429f;
	float p = 0.3275911f;

	// Save the sign of x
	int sign = 1;
	if (x < 0.f) sign = -1;
	x = std::fabs(x);

	// A&S formula 7.1.26
	float t = 1.f / (1.f + p * x);
	float y = 1.f - (((((a5 * t + a4) * t) + a3) * t + a2) * t + a1) * t * std::exp(-x * x);

	return sign * y;
}

inline vec3 sphericalDirection(float sinTheta, float cosTheta, float phi)
{
	return vec3(sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta);
}

inline bool sameHemisphere(const vec3 &w, const vec3 &wp)
{
	return w.z * wp.z > 0.f;
}

// Microfacet Utility Functions
static vec2 beckmannSample11(float cosThetaI, float U1, float U2)
{
	// Special case (normal incidence)
	if (cosThetaI > 0.9999f) {
		float r = std::sqrt(-std::log(std::max(1.f - U1, 1e-6f)));
		float phi = M_2PIf * U2;
		return vec2(r * std::cos(phi), r * std::sin(phi));
	}

	// The original inversion routine from the paper contained
	// discontinuities, which causes issues for QMC integration
	// and techniques like Kelemen-style MLT. The following code
	// performs a numerical inversion with better behavior
	float sinThetaI = std::sqrt(std::max(0.f, 1.f - cosThetaI * cosThetaI));
	float tanThetaI = sinThetaI / cosThetaI;
	float cotThetaI = 1.f / tanThetaI;

	// Search interval -- everything is parameterized in the Erf() domain
	float a = -1.f, c = Erf(cotThetaI);
	float sample_x = std::max(U1, 1e-6f);

	// Start with a good initial guess
	// float b = (1-sample_x) * a + sample_x * c;

	// We can do better (inverse of an approximation computed in Mathematica)
	float thetaI = std::acos(cosThetaI);
	float fit = 1.f + thetaI * (-0.876f + thetaI * (0.4265f - 0.0594f * thetaI));
	float b = c - (1.f + c) * std::pow(1.f - sample_x, fit);

	// Normalization factor for the CDF
	static const float SQRT_PI_INV = 1.f / std::sqrt(M_PIf);
	float normalization = 1.f / (1.f + c + SQRT_PI_INV * tanThetaI * std::exp(-cotThetaI * cotThetaI));

	int it = 0;
	while (++it < 10) {
		// Bisection criterion -- the oddly-looking
		// Boolean expression are intentional to check
		// for NaNs at little additional cost
		if (!(b >= a && b <= c))
			b = 0.5f * (a + c);

		// Evaluate the CDF and its derivative (i.e. the density function)
		float invErf = erfInv(b);
		float value = normalization * (1.f + b + SQRT_PI_INV * tanThetaI * std::exp(-invErf * invErf)) - sample_x;
		float derivative = normalization * (1.f - invErf * tanThetaI);

		if (std::fabs(value) < 1e-5f)
			break;

		// Update bisection intervals
		if (value > 0)
			c = b;
		else
			a = b;

		b -= value / derivative;
	}

	vec2 slope;

	// Now convert back into a slope value
	slope.x = erfInv(b);
	assert(isfinite(slope.x));

	// Simulate Y component
	slope.y = erfInv(2.f * std::max(U2, 1e-6f) - 1.f);
	assert(isfinite(slope.y));

	return slope;
}

static vec3 beckmannSample(const vec3 &wi, const vec2 & alpha, float U1, float U2)
{
	// 1. stretch wi
	auto wiStretched = glm::normalize(vec3(alpha.x * wi.x, alpha.y * wi.y, wi.z));

	// 2. simulate P22_{wi}(x_slope, y_slope, 1, 1)
	auto slope = beckmannSample11(cosTheta(wiStretched), U1, U2);

	// 3. rotate
	slope = vec2(cosPhi(wiStretched) * slope.x - sinPhi(wiStretched) * slope.y,
				 sinPhi(wiStretched) * slope.x + cosPhi(wiStretched) * slope.y);

	// 4. unstretch
	slope.x *= alpha.x;
	slope.y *= alpha.y;

	// 5. compute normal
	return glm::normalize(vec3(-slope.x, -slope.y, 1.f));
}

// MicrofacetDistribution Method Definitions
float BeckmannDistribution::d(const vec3 &wh) const
{
	const auto t2t = tan2Theta(wh);
	if (std::isinf(t2t)) return 0.f;
	const auto cos4Theta = cos2Theta(wh) * cos2Theta(wh);
	return std::exp(-t2t * (cos2Phi(wh) / (m_alpha.x * m_alpha.x) +
								  sin2Phi(wh) / (m_alpha.y * m_alpha.y))) /
								(M_PIf * m_alpha.x * m_alpha.y * cos4Theta);
}

float TrowbridgeReitzDistribution::d(const vec3 &wh) const
{
	const auto t2t = tan2Theta(wh);
	if (std::isinf(t2t)) return 0.f;
	const auto cos4Theta = cos2Theta(wh) * cos2Theta(wh);
	const auto e = (cos2Phi(wh) / (m_alpha.x * m_alpha.x) + sin2Phi(wh) / (m_alpha.y * m_alpha.y)) * t2t;
	return 1.f / (M_PIf * m_alpha.x * m_alpha.y * cos4Theta * (1.f + e) * (1.f + e));
}

float BeckmannDistribution::lambda(const vec3 &w) const
{
	float absTanTheta = std::fabs(tanTheta(w));
	if (std::isinf(absTanTheta)) return 0.;
	// Compute _alpha_ for direction _w_
	float alpha = std::sqrt(cos2Phi(w) * m_alpha.x * m_alpha.x + sin2Phi(w) * m_alpha.y * m_alpha.y);
	float a = 1.f / (alpha * absTanTheta);
	if (a >= 1.6f) return 0.f;
	return (1.f - 1.259f * a + 0.396f * a * a) / (3.535f * a + 2.181f * a * a);
}

float TrowbridgeReitzDistribution::lambda(const vec3 &w) const
{
	float absTanTheta = std::fabs(tanTheta(w));
	if (std::isinf(absTanTheta)) return 0.f;
	// Compute _alpha_ for direction _w_
	float alpha = std::sqrt(cos2Phi(w) * m_alpha.x * m_alpha.x + sin2Phi(w) * m_alpha.y * m_alpha.y);
	float alpha2Tan2Theta = (alpha * absTanTheta) * (alpha * absTanTheta);
	return (-1.f + std::sqrt(1.f + alpha2Tan2Theta)) / 2.f;
}

vec3 BeckmannDistribution::sample_wh(const vec3 &wo, const vec2 &u) const
{
	if (!m_sampleVisibleArea) {
		// Sample full distribution of normals for Beckmann distribution

		// Compute $\tan^2 \theta$ and $\phi$ for Beckmann distribution sample
		float tan2Theta, phi;
		if (m_alpha.x == m_alpha.y) {
			float logSample = std::log(u.x);
			if (std::isinf(logSample)) logSample = 0.f;
			tan2Theta = -m_alpha.x * m_alpha.x * logSample;
			phi = u.y * M_2PIf;
		} else {
			// Compute _tan2Theta_ and _phi_ for anisotropic Beckmann
			// distribution
			float logSample = std::log(u.x);
			if (std::isinf(logSample)) logSample = 0.f;
			phi = std::atan(m_alpha.y / m_alpha.x * std::tan(M_2PIf * u.y + M_HALF_PIf));
			if (u.y > 0.5f) phi += M_PIf;
			float sinPhi = std::sin(phi), cosPhi = std::cos(phi);
			float alphax2 = m_alpha.x * m_alpha.x, alphay2 = m_alpha.y * m_alpha.y;
			tan2Theta = -logSample /
			(cosPhi * cosPhi / alphax2 + sinPhi * sinPhi / alphay2);
		}

		// Map sampled Beckmann angles to normal direction _wh_
		float cosTheta = 1.f / std::sqrt(1 + tan2Theta);
		float sinTheta = std::sqrt(std::max(0.f, 1.f - cosTheta * cosTheta));
		vec3 wh = sphericalDirection(sinTheta, cosTheta, phi);
		if (!sameHemisphere(wo, wh)) wh = -wh;

		return wh;
	} else {
		// Sample visible area of normals for Beckmann distribution
		vec3 wh;
		bool flip = wo.z < 0.f;
		wh = beckmannSample(flip ? -wo : wo, m_alpha, u.x, u.y);

		if (flip) wh = -wh;
		return wh;
	}
}

static vec2 TrowbridgeReitzSample11(float cosTheta, float U1, float U2)
{
	// special case (normal incidence)
	if (cosTheta > 0.9999f) {
		float r = std::sqrt(U1 / std::max(1.f - U1, 1e-6f));
		float phi = M_2PIf * U2;
		return vec2(r * std::cos(phi), r * std::sin(phi));
	}

	float sinTheta = std::sqrt(std::max(0.f, 1.f - cosTheta * cosTheta));
	float tan_theta_i = sinTheta / cosTheta;
	float a = 1.f / tan_theta_i;
	float G1 = 2.f / (1.f + std::sqrt(1.f + 1.f / (a * a)));

	// sample slope_x
	float A = 2.f * U1 / G1 - 1.f;
	float tmp = 1.f / (A * A - 1.f);
	if (tmp > 1e10f) tmp = 1e10f;
	float B = tan_theta_i;
	float D = std::sqrt(std::max(float(B * B * tmp * tmp - (A * A - B * B) * tmp), 0.f));
	float slope_x_1 = B * tmp - D;
	float slope_x_2 = B * tmp + D;

	vec2 slope;

	slope.x = (A < 0.f || slope_x_2 > 1.f / tan_theta_i) ? slope_x_1 : slope_x_2;
	assert(isfinite(slope.x));

	// sample slope_y
	float S;
	if (U2 > 0.5f) {
		S = 1.f;
		U2 = 2.f * (U2 - .5f);
	} else {
		S = -1.f;
		U2 = 2.f * (.5f - U2);
	}
	float z = (U2 * (U2 * (U2 * 0.27385f - 0.73369f) + 0.46341f)) /
				(U2 * (U2 * (U2 * 0.093073f + 0.309420f) - 1.000000f) + 0.597999f);
	slope.y = S * z * std::sqrt(1.f + slope.x * slope.x);
	assert(isfinite(slope.y));

	return slope;
}

static vec3 TrowbridgeReitzSample(const vec3 &wi, const vec2 & alpha, float U1, float U2)
{
	// 1. stretch wi
	auto wiStretched = glm::normalize(vec3(alpha.x * wi.x, alpha.y * wi.y, wi.z));

	// 2. simulate P22_{wi}(x_slope, y_slope, 1, 1)
	auto slope = TrowbridgeReitzSample11(cosTheta(wiStretched), U1, U2);

	// 3. rotate
	slope = vec2(cosPhi(wiStretched) * slope.x - sinPhi(wiStretched) * slope.y,
				 sinPhi(wiStretched) * slope.x + cosPhi(wiStretched) * slope.y);

	// 4. unstretch
	slope.x *= alpha.x;
	slope.y *= alpha.y;

	// 5. compute normal
	return glm::normalize(vec3(-slope.x, -slope.y, 1.f));
}

vec3 TrowbridgeReitzDistribution::sample_wh(const vec3 &wo, const vec2 &u) const
{
	vec3 wh;
	if (!m_sampleVisibleArea) {
		float cosTheta = 0.f, phi = M_2PIf * u.y;
		if (m_alpha.x == m_alpha.y) {
			float tanTheta2 = m_alpha.x * m_alpha.x * u.x / (1.f - u.x);
			cosTheta = 1.f / std::sqrt(1 + tanTheta2);
		} else {
			phi = std::atan(m_alpha.y / m_alpha.x * std::tan(M_2PIf * u.y + M_HALF_PIf));
			if (u.y > 0.5f) phi += M_PIf;
			float sinPhi = std::sin(phi), cosPhi = std::cos(phi);
			const float alphax2 = m_alpha.x * m_alpha.x, alphay2 = m_alpha.y * m_alpha.y;
			const float alpha2 = 1.f / (cosPhi * cosPhi / alphax2 + sinPhi * sinPhi / alphay2);
			float tanTheta2 = alpha2 * u.x / (1.f - u.x);
			cosTheta = 1.f / std::sqrt(1.f + tanTheta2);
		}
		float sinTheta = std::sqrt(std::max(0.f, 1.f - cosTheta * cosTheta));
		wh = sphericalDirection(sinTheta, cosTheta, phi);
		if (!sameHemisphere(wo, wh)) wh = -wh;
	} else {
		bool flip = wo.z < 0.f;
		wh = TrowbridgeReitzSample(flip ? -wo : wo, m_alpha, u.x, u.y);
		if (flip) wh = -wh;
	}
	return wh;
}

float MicrofacetDistribution::pdf(const vec3 &wo, const vec3 &wh) const
{
	if (m_sampleVisibleArea)
		return d(wh) * g1(wo) * std::fabs(glm::dot(wo, wh)) / abscosTheta(wo);
	else
		return d(wh) * abscosTheta(wh);
}
