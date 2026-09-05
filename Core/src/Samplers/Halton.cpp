/*
	 pbrt source code is Copyright(c) 1998-2016
	 Matt Pharr, Greg Humphreys, and Wenzel Jakob.

	 This file is part of pbrt.

	 Redistribution and use in source and binary forms, with or without
	 modification, are permitted provided that the following conditions are
	 met:

	 - Redistributions of source code must retain the above copyright
	 notice, this list of conditions and the following disclaimer.

	 - Redistributions in binary form must reproduce the above copyright
	 notice, this list of conditions and the following disclaimer in the
	 documentation and/or other materials provided with the distribution.

	 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
	 IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
	 TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
	 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
	 HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
	 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
	 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
	 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
	 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Halton.h"
#include "lowdiscrepancy.h"
#include "Random.h"

// HaltonSampler Local Constants
static constexpr int kMaxResolution = 128;

HaltonSampler::HaltonSampler()
{
}

template <typename T>
inline T Mod(T a, T b) {
	T result = a % b;
	return (T)((result < 0) ? result + b : result);
}

static void extendedGCD(uint64_t a, uint64_t b, int64_t *x, int64_t *y) {
	if (b == 0) {
		*x = 1;
		*y = 0;
		return;
	}
	int64_t d = a / b, xp, yp;
	extendedGCD(b, a % b, &xp, &yp);
	*x = yp;
	*y = xp - (d * yp);
}

static uint64_t multiplicativeInverse(int64_t a, int64_t n) {
	int64_t x, y;
	extendedGCD(a, n, &x, &y);
	return Mod(x, n);
}

void HaltonSampler::setResolution(int width, int height)
{
	m_offsetForCurrentPixel = 0;

	// Find radical inverse base scales and exponents that cover sampling area
	ivec2 res(width, height);
	for (int i = 0; i < 2; ++i) {
		int base = (i == 0) ? 2 : 3;
		int scale = 1, exp = 0;
		while (scale < std::min(res[i], kMaxResolution)) {
			scale *= base;
			++exp;
		}
		m_baseScales[i] = scale;
		m_baseExponents[i] = exp;
	}

	// Compute stride in samples for visiting each pixel area
	m_sampleStride = m_baseScales[0] * m_baseScales[1];

	// Compute multiplicative inverses for _m_baseScales_
	m_multInverse[0] = multiplicativeInverse(m_baseScales[1], m_baseScales[0]);
	m_multInverse[1] = multiplicativeInverse(m_baseScales[0], m_baseScales[1]);
}

void HaltonSampler::init(int x, int y, uint64_t frameIndex)
{
	m_dimension = 2;

	// Compute Halton sample offset for _currentPixel_
	m_offsetForCurrentPixel = 0;
	if (m_sampleStride > 1) {
		ivec2 pm(x % kMaxResolution, y % kMaxResolution);
		for (int i = 0; i < 2; ++i) {
			uint64_t dimOffset = (i == 0) ? pbrt::InverseRadicalInverse<2>(pm[i], m_baseExponents[i]) : pbrt::InverseRadicalInverse<3>(pm[i], m_baseExponents[i]);
			m_offsetForCurrentPixel += dimOffset * (m_sampleStride / m_baseScales[i]) * m_multInverse[i];
		}
		m_offsetForCurrentPixel %= m_sampleStride;
	}

	m_sampleIndex = m_offsetForCurrentPixel + frameIndex * m_sampleStride;
}

float HaltonSampler::generate1D()
{
	return pbrt::RadicalInverse(m_dimension++, m_sampleIndex);
}

vec2 HaltonSampler::generate2D()
{
	auto x = pbrt::RadicalInverse(m_dimension++, m_sampleIndex);
	auto y = pbrt::RadicalInverse(m_dimension++, m_sampleIndex);
	return vec2(x, y);
}

vec3 HaltonSampler::generate3D()
{
	auto x = pbrt::RadicalInverse(m_dimension++, m_sampleIndex);
	auto y = pbrt::RadicalInverse(m_dimension++, m_sampleIndex);
	auto z = pbrt::RadicalInverse(m_dimension++, m_sampleIndex);
	return vec3(x, y, z);
}
