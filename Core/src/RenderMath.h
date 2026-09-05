/*
	Copyright (C) 2013-2020 Damir Sagidullin

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
	documentation files (the "Software"), to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions
	of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
	TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
	THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
	DEALINGS IN THE SOFTWARE.

	(The above is MIT License: http://en.wikipedia.origin/wiki/MIT_License)
*/

#pragma once

#define M_PI_4f		0.785398163397448309615660845819875721f		// pi / 4

#define M_2_PIf		0.636619772367581343075535053490057448f		// 2 / pi
#define M_1_PIf		0.318309886183790671537767526745028724f		// 1 / pi
#define M_1_2PIf	0.159154943091895335768883763372514362f		// 1 / 2pi
#define M_1_4PIf	0.079577471545947667884441881686257181f		// 1 / 4pi

#define EPS_COSINE	1e-6f
#define EPS_COLOR	1e-4f

#ifdef _WIN32
#define ONE_MINUS_EPS_FLT 0.999999940395355225f
#define ONE_MINUS_EPS_DBL 0.999999999999999888
#define RCPOVERFLOW_FLT   2.93873587705571876e-39f
#define RCPOVERFLOW_DBL   5.56268464626800345e-309
#else
#define ONE_MINUS_EPS_FLT 0x1.fffffep-1f
#define ONE_MINUS_EPS_DBL 0x1.fffffffffffff7p-1
#define RCPOVERFLOW_FLT   0x1p-128f
#define RCPOVERFLOW_DBL   0x1p-1024
#endif

// ------------------------------------------------------------------------ //

typedef std::complex<float>		Complex;

inline float fresnelReflectanceScalar(float n1, float n2, float cosI)
{
	if (n1 == n2)
		return 0.f;

	if (cosI <= 0.f)
		return 1.f; // total internal reflection at zero grazing angle

	cosI = std::min(cosI, 1.f);
	float eta = n1 / n2;
	float sinI2 = 1.f - cosI * cosI;
	float sinT2 = eta * eta * sinI2;

	if (sinT2 >= 1.f)
		return 1.f; // total internal reflection

	float cosT = std::sqrt(std::min(1.f - sinT2, 1.f));

	float eta_cosI = eta * cosI;
	float eta_cosT = eta * cosT;

	float Rs = (eta_cosI - cosT) / (eta_cosI + cosT);
	float Rp = (eta_cosT - cosI) / (eta_cosT + cosI);

	float res = ((Rs * Rs) + (Rp * Rp)) * 0.5f;
	assert(!isnan(res));
	return res;
}

inline float fresnelReflectanceComplex(const Complex & nO, const Complex & nM, float cosI)
{
	if (cosI <= 0.f)
		return 1.f; // total internal reflection at zero grazing angle

	cosI = std::min(cosI, 1.f);
	Complex cosO(cosI, 0.f);
	float sinO = std::sqrt(1.f - cosI * cosI);

	Complex sinM = (nO * sinO) / nM;
	Complex cosM = std::sqrt(Complex(1.f, 0.f) - sinM * sinM);

	Complex nO_cosO = nO * cosI;
	Complex nM_cosM = nM * cosM;
	Complex nM_cosO = nM * cosI;
	Complex nO_cosM = nO * cosM;

	Complex rs_den = nO_cosO + nM_cosM;
	Complex rs = rs_den != 0.f ? (nO_cosO - nM_cosM) / rs_den : 0.f;

	Complex rp_den = nM_cosO + nO_cosM;
	Complex rp = rp_den != 0.f ? (nM_cosO - nO_cosM) / rp_den : 0.f;

	float R_S = std::norm(rs);
	float R_P = std::norm(rp);

	return std::min(0.5f * (R_S + R_P), 1.f);
}

inline float fresnelReflectance(const Complex & n1, const Complex & n2, float cosI)
{
	if (n1 == n2)
		return 0.f;

	if (cosI <= 0.f)
		return 1.f;

	if (n1.imag() == 0.f && n2.imag() == 0.f)
		return fresnelReflectanceScalar(n1.real(), n2.real(), cosI);

	return fresnelReflectanceComplex(n1, n2, cosI);
}

class IndexOfRefractionImpl;

vec3 getFresnelReflectance(float cosI, const IndexOfRefractionImpl * ior1, const IndexOfRefractionImpl * ior2, int offset = -1);

vec3 getThinFilmReflectance(float & averageReflectance, float cosI, float thickness, int offset,
							const IndexOfRefractionImpl * ior0, const IndexOfRefractionImpl * ior1, const IndexOfRefractionImpl * ior2);

float getThinFilmReflectance(const Complex & nO, const Complex & nF, const Complex & nM, float thickness, float cosI, float waveLength);

// ------------------------------------------------------------------------ //

vec3 xyzToRGB(const vec3 & xyz);

// ------------------------------------------------------------------------ //

inline vec2 uniformSampleTriangle(float u1, float u2)
{
	auto su1 = std::sqrt(u1);
	return vec2(1.f - su1, u2 * su1);
}

inline vec2 uniformSampleDisk(float u1, float u2)
{
	auto r = std::sqrt(u1);
	auto theta = M_2PIf * u2;
	return vec2(r * std::cos(theta), r * std::sin(theta));
}

inline vec2 concentricSampleDisk(float u1, float u2)
{
	float phi, r;

	auto a = u1 * 2.f - 1.f;   // (a,b) is now on [-1,1]^2
	auto b = u2 * 2.f - 1.f;

	if (a > -b) { // region 1 or 2
		if (a > b) { // region 1, also |a| > |b|
			r = a;
			phi = (b / a);
		} else { // region 2, also |b| > |a|
			r = b;
			phi = (2.f - (a / b));
		}
	} else { // region 3 or 4
		if(a < b) { // region 3, also |a| >= |b|, a != 0
			r = -a;
			phi = (4.f + (b / a));
		} else { // region 4, |b| >= |a|, but a==0 and b==0 could occur.
			r = -b;

			if (b != 0)
				phi = (6.f - (a / b));
			else
				phi = 0;
		}
	}

	phi *= M_PI_4f;
	return vec2(r * std::cos(phi), r * std::sin(phi));
}

inline float concentricDiskPdfA()
{
	return M_1_PIf;
}

inline vec3 uniformSampleHemisphere(float u1, float u2)
{
	auto z = u1;
	auto r = std::sqrt(1.f - z * z);
	auto phi = M_2PIf * u2;
	return vec3(r * std::cos(phi), r * std::sin(phi), z);
}

inline float uniformHemispherePdfW()
{
	return M_1_2PIf;
}

inline vec3 uniformSampleSphere(float u1, float u2)
{
	auto phi = M_2PIf * u1;
	auto z = 1.f - 2.f * u2;
	auto r = std::sqrt(1.f - z * z);
	return vec3(r * cosf(phi), r * sinf(phi), z);

	//const float term1 = M_2PIf * u1;
	//const float term2 = 2.f * std::sqrt(u2 - u2 * u2);
	//return vec3(std::cos(term1) * term2, std::sin(term1) * term2, 1.f - 2.f * u2);
}

inline float uniformSpherePdfW()
{
	return M_1_4PIf;
}

inline float uniformConePdf(float cosThetaMax)
{
	return 1.f / (M_2PIf * (1.f - cosThetaMax));
}

inline vec3 cosineSampleHemisphere(const vec2 & u, float power, float * pdfW)
{
	auto phi = M_2PIf * u.x;
	auto cosTheta = std::pow(u.y, 1.f / (power + 1.f));
	auto sinTheta = std::sqrt(1.f - cosTheta * cosTheta);

	if (pdfW)
		*pdfW = (power + 1.f) * std::pow(cosTheta, power) * M_1_2PIf;

	return vec3(std::cos(phi) * sinTheta, std::sin(phi) * sinTheta, cosTheta);
}

inline vec3 cosineSampleHemisphere(const vec2 & u, float * pdfW)
{// power = 1
	auto phi = M_2PIf * u.x;
	auto cosTheta = std::sqrt(u.y);
	auto sinTheta = std::sqrt(1.f - u.y);

	if (pdfW)
		*pdfW = cosTheta * M_1_PIf;

	return vec3(std::cos(phi) * sinTheta, std::sin(phi) * sinTheta, cosTheta);
}

inline float cosineHemispherePdfW(const vec3 & normal, const vec3 & direction)
{
	return std::max(0.f, glm::dot(normal, direction)) * M_1_PIf;
}

class Random;
class Sampler;

vec2 rejectionSampleDisk(Sampler & sampler);
vec3 rejectionSampleBall(Sampler & sampler);
vec2 uniformSampleTriangle(Sampler & sampler);
vec2 uniformSampleDisk(Sampler & sampler);
vec2 concentricSampleDisk(Sampler & sampler);
vec3 uniformSampleHemisphere(Sampler & sampler);
vec3 uniformSampleSphere(Sampler & sampler);

//inline float VanDerCorput(uint32 n, uint32 scramble)
//{
//	n = (n << 16) | (n >> 16);
//	n = ((n & 0x00ff00ff) << 8) | ((n & 0xff00ff00) >> 8);
//	n = ((n & 0x0f0f0f0f) << 4) | ((n & 0xf0f0f0f0) >> 4);
//	n = ((n & 0x33333333) << 2) | ((n & 0xcccccccc) >> 2);
//	n = ((n & 0x55555555) << 1) | ((n & 0xaaaaaaaa) >> 1);
//	n ^= scramble;
//	return ((n >> 8) & 0xffffff) / float(1 << 24);
//}
//
//inline float radicalInverse_VdC(uint32 bits)
//{
//	bits = (bits << 16u) | (bits >> 16u);
//	bits = ((bits & 0x55555555) << 1u) | ((bits & 0xAAAAAAAA) >> 1u);
//	bits = ((bits & 0x33333333) << 2u) | ((bits & 0xCCCCCCCC) >> 2u);
//	bits = ((bits & 0x0F0F0F0F) << 4u) | ((bits & 0xF0F0F0F0) >> 4u);
//	bits = ((bits & 0x00FF00FF) << 8u) | ((bits & 0xFF00FF00) >> 8u);
//	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
//}

// ------------------------------------------------------------------------ //

inline float cosTheta(const vec3 &w) { return w.z; }
inline float cos2Theta(const vec3 &w) { return w.z * w.z; }
inline float abscosTheta(const vec3 &w) { return std::fabs(w.z); }

inline float sin2Theta(const vec3 &w) { return std::max(0.f, 1.f - cos2Theta(w)); }
inline float sinTheta(const vec3 &w) { return std::sqrt(sin2Theta(w)); }

inline float tanTheta(const vec3 &w) { return sinTheta(w) / cosTheta(w); }
inline float tan2Theta(const vec3 &w) { return sin2Theta(w) / cos2Theta(w); }

inline float cosPhi(const vec3 &w)
{
	const auto st = sinTheta(w);
	return (st == 0.f) ? 1.f : std::clamp(w.x / st, -1.f, 1.f);
}

inline float sinPhi(const vec3 &w)
{
	const auto st = sinTheta(w);
	return (st == 0.f) ? 0.f : std::clamp(w.y / st, -1.f, 1.f);
}

inline float cos2Phi(const vec3 &w) { return cosPhi(w) * cosPhi(w); }

inline float sin2Phi(const vec3 &w) { return sinPhi(w) * sinPhi(w); }

// ------------------------------------------------------------------------ //

inline float minComponent(const vec3 & v)
{
	return std::min(std::min(v.x, v.y), v.z);
}

inline float maxComponent(const vec3 & v)
{
	return std::max(std::max(v.x, v.y), v.z);
}

inline float maxComponent(const vec2 & v)
{
	return std::max(v.x, v.y);
}

inline float luminance(const vec3 & rgb)
{
	return glm::dot(rgb, vec3(0.212671f, 0.715160f, 0.072169f));
}

inline bool isBlack(const vec3 & c)
{
	return c.r <= EPS_COLOR && c.g <= EPS_COLOR && c.b <= EPS_COLOR;
}

inline bool isWhite(const vec3 & c)
{
	return c.r >= (1.f - EPS_COLOR) && c.g >= (1.f - EPS_COLOR) && c.b >= (1.f - EPS_COLOR);
}

inline bool isRoughlyNormalized(const vec3 & n)
{
	return std::fabs(glm::length2(n) - 1.f) < 0.01f;
}

inline bool isIsotropic(float aMeanCosine)
{
	return std::fabs(aMeanCosine) < 1e-3f;
}

inline vec2 cylindricalTexCoords(float x, float y, float z)
{
	return vec2(std::atan2(x, y) * M_1_2PIf, z);
}

inline vec2 sphericalTexCoords(float x, float y, float z)
{
	return vec2(std::atan2(x, y) * M_1_2PIf, std::atan2(std::sqrt(x * x + y * y), z) * M_1_PIf);
}

inline vec2 sphericalTexCoords(const vec3 & dir)
{// spherical environment map
	return sphericalTexCoords(-dir.x, -dir.y, -dir.z);
}

inline vec2 fishEyeTexCoords(const vec3 & dir)
{
	// fisheye environment map
	float l = glm::length(vec2(dir.x, dir.z));
	float d = (std::atan2(dir.y, l) * M_1_2PIf + 0.25f);
	if (l > 0.f)
		d /= l;

	return vec2(dir.x * d + 0.5f, dir.z * d + 0.5f);
}

inline vec2 planarMappingX(const vec3 & texPos)
{
	return vec2(1.f - texPos.y, texPos.z);
}

inline vec2 planarMappingY(const vec3 & texPos)
{
	return vec2(texPos.x, texPos.z);
}

inline vec2 planarMappingZ(const vec3 & texPos)
{
	return vec2(texPos.x, texPos.y);
}

inline vec2 cylindricalMappingX(const vec3 & texPos)
{
	return cylindricalTexCoords(texPos.y * 2.f - 1.f, texPos.z * -2.f + 1.f, texPos.x);
}

inline vec2 cylindricalMappingY(const vec3 & texPos)
{
	return cylindricalTexCoords(texPos.x * -2.f + 1.f, texPos.z * -2.f + 1.f, texPos.y);
}

inline vec2 cylindricalMappingZ(const vec3 & texPos)
{
	return cylindricalTexCoords(texPos.x * -2.f + 1.f, texPos.y * 2.f - 1.f, texPos.z);
}

inline vec2 sphericalMappingX(const vec3 & texPos)
{
	return sphericalTexCoords(texPos.y * 2.f - 1.f, texPos.z * -2.f + 1.f, texPos.x * -2.f + 1.f);
}

inline vec2 sphericalMappingY(const vec3 & texPos)
{
	return sphericalTexCoords(texPos.x * -2.f + 1.f, texPos.z * -2.f + 1.f, texPos.y * -2.f + 1.f);
}

inline vec2 sphericalMappingZ(const vec3 & texPos)
{
	return sphericalTexCoords(texPos.x * -2.f + 1.f, texPos.y * 2.f - 1.f, texPos.z * -2.f + 1.f);
}

// Monte Carlo Inline Functions
inline float balanceHeuristic(int nf, float pdfF, int ng, float pdfG)
{
	return (nf * pdfF) / (nf * pdfF + ng * pdfG);
}

inline float powerHeuristic(int nf, float pdfF, int ng, float pdfG)
{
	float f = nf * pdfF, g = ng * pdfG;
	return (f*f) / (f*f + g*g);
}

// ------------------------------------------------------------------------ //
// Utilities for converting PDF between Area (A) and Solid angle (W)
// WtoA = PdfW * cosine / distance_squared
// AtoW = PdfA * distance_squared / cosine

inline float pdfWtoA(float aPdfW, float aDist, float aCosThere)
{
	return aPdfW * std::fabs(aCosThere) / sqr(aDist);
}

inline float pdfAtoW(float aPdfA, float aDist, float aCosThere)
{
	return aPdfA * sqr(aDist) / std::fabs(aCosThere);
}

// ------------------------------------------------------------------------ //

class Frame
{
public:
	Frame() : mX(1.f, 0.f, 0.f), mY(0.f, 1.f, 0.f), mZ(0.f, 0.f, 1.f) {}
	Frame(const vec3 & x, const vec3 & y, const vec3 & z) : mX(x), mY(y), mZ(z) {}

	void setFromZ(const vec3 & z)
	{
		mZ = z;
		mX = glm::perpendicular(z);
		mY = glm::cross(mZ, mX);
	}

	vec3 toWorld(const vec3& a) const
	{
		return mX * a.x + mY * a.y + mZ * a.z;
	}

	vec3 toLocal(const vec3& a) const
	{
		return vec3(glm::dot(a, mX), glm::dot(a, mY), glm::dot(a, mZ));
	}

	const vec3 & binormal() const { return mX; }
	const vec3 & tangent() const { return mY; }
	const vec3 & normal() const { return mZ; }

public:

	vec3 mX, mY, mZ;
};

// ------------------------------------------------------------------------ //

inline vec2 getBarycentricCoords(const vec3 & delta, const vec3 & dpdu, const vec3 & dpdv)
{
	auto uu = glm::dot(dpdu, dpdu);
	auto uv = glm::dot(dpdu, dpdv);
	auto vv = glm::dot(dpdv, dpdv);
	auto d = (uv * uv - uu * vv);
	if (d == 0.f) return vec2(0.f);

	auto wu = glm::dot(delta, dpdu);
	auto wv = glm::dot(delta, dpdv);

	return vec2(uv * wv - vv * wu, uv * wu - uu * wv) / d;
}

// ------------------------------------------------------------------------ //

inline int countTrailingZeros(uint32_t v) {
#if defined(_MSC_VER)
	unsigned long index;
	if (_BitScanForward(&index, v))
		return index;
	else
		return 32;
#else
	return __builtin_ctz(v);
#endif
}

inline int log2i(uint32_t value)
{
	int r = 0;
	while ((value >> r) != 0)
		r++;
	return r - 1;
}

/* Fast rounding & power-of-two test algorithms from PBRT */
inline uint32_t roundToPowerOfTwo(uint32_t i)
{
	i--;
	i |= i >> 1; i |= i >> 2;
	i |= i >> 4; i |= i >> 8;
	i |= i >> 16;
	return i + 1;
}

// Tiny Encryption Algorithm (TEA) to calculate a the seed per launch index and iteration.
template<unsigned int N>
inline uint64_t tea(uint32_t v0, uint32_t v1)
{
	uint32_t sum = 0;

	for (int i = 0; i < N; ++i) {
		sum += 0x9e3779b9;
		v0 += ((v1 << 4) + 0xA341316C) ^ (v1 + sum) ^ ((v1 >> 5) + 0xC8013EA4);
		v1 += ((v0 << 4) + 0xAD90777D) ^ (v0 + sum) ^ ((v0 >> 5) + 0x7E95761E);
	}

	return ((uint64_t)v1 << 32) + v0;
}

inline vec2 angleToDir(float degrees)
{
	const auto rad = glm::radians(degrees);
	return vec2(std::cos(rad), std::sin(rad));
}

// ------------------------------------------------------------------------ //

struct Ray;
class TextureImpl;
class TextureScalarParameterImpl;

vec3 getBumpNormal(const vec3 & dpdx, const vec3 & dpdy, const vec3 & dpdz, const TextureImpl * normalMap, const vec2 & tc);
void traceBumpMap(Ray & ray, TextureScalarParameterImpl & bump, int numLinearSearchSteps, int numBinarySearchSteps);
bool traceBumpShadow(const vec3 & dir, const Ray & ray, int numLinearSearchSteps, float offset);
bool traceBumpInside(const vec3 & dir, Ray & ray, int numLinearSearchSteps, int numBinarySearchSteps, float offset);

