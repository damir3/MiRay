/*
	Copyright (C) 2015-2020 Damir Sagidullin

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

#include "Sobol.h"
#include "lowdiscrepancy.h"

SobolSampler::SobolSampler()
	: m_scramble(tea<4>(0x12345678, 0))
	, m_resolution(1)
	, m_logResolution(0)
{
}

void SobolSampler::setResolution(int width, int height)
{
	uint32_t resolution = roundToPowerOfTwo((uint32_t)std::max(width, height));

	m_resolution = (float)resolution;
	m_logResolution = log2i(resolution);
}

void SobolSampler::init(int x, int y, uint64_t frameIndex)
{
	m_dimension = 2;
	m_sampleIndex = pbrt::SobolIntervalToIndex(m_logResolution, frameIndex, ivec2(x, y));
}

float SobolSampler::generate1D()
{
	return pbrt::SobolSample(m_sampleIndex, m_dimension++, m_scramble);
}

vec2 SobolSampler::generate2D()
{
	auto x = pbrt::SobolSample(m_sampleIndex, m_dimension++, m_scramble);
	auto y = pbrt::SobolSample(m_sampleIndex, m_dimension++, m_scramble);
	return vec2(x, y);
}

vec3 SobolSampler::generate3D()
{
	auto x = pbrt::SobolSample(m_sampleIndex, m_dimension++, m_scramble);
	auto y = pbrt::SobolSample(m_sampleIndex, m_dimension++, m_scramble);
	auto z = pbrt::SobolSample(m_sampleIndex, m_dimension++, m_scramble);
	return vec3(x, y, z);
}
