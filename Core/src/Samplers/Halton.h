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

#pragma once

#include "Sampler.h"

class HaltonSampler final : public Sampler
{
	uint32_t	m_dimension;
	uint64_t	m_sampleIndex;

	int64_t 	m_baseScales[2];
	ivec2		m_baseExponents;
	int64_t 	m_sampleStride;
	int64_t		m_multInverse[2];
	int64_t 	m_offsetForCurrentPixel;

public:
	HaltonSampler();

	void setResolution(int width, int height) override;
	void init(int x, int y, uint64_t frameIndex) override;

	float generate1D() override;
	vec2 generate2D() override;
	vec3 generate3D() override;
};
