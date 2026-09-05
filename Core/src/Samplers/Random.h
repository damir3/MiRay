/*
	Copyright (C) 2015 Damir Sagidullin

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
#include <random>

class Random final : public Sampler
{
	std::mt19937 m_gen;
	std::uniform_real_distribution<float>   m_realDist;
	std::uniform_real_distribution<float>   m_real2Dist;
	std::uniform_int_distribution<int32_t>  m_intDist;
	std::uniform_int_distribution<uint32_t> m_uintDist;

public:
	Random()
		: m_gen(std::random_device()())
		, m_realDist(0.f, 1.f)
		, m_real2Dist(-1.f, 1.f)
		, m_intDist()
		, m_uintDist()
	{
	}

	void setSeed(int seed)
	{
		m_gen.seed(seed);
	}

	float generate1D() override // returns uniform random float [0; 1]
	{
		return m_realDist(m_gen);
	}

	vec2 generate2D() override // returns uniform random vector ([0; 1], [0; 1])
	{
		auto x = m_realDist(m_gen);
		auto y = m_realDist(m_gen);
		return vec2(x, y);
	}

	vec3 generate3D() override // returns uniform random vector ([0; 1], [0; 1], [0; 1])
	{
		auto x = m_realDist(m_gen);
		auto y = m_realDist(m_gen);
		auto z = m_realDist(m_gen);
		return vec3(x, y, z);
	}

	float signedRand() // returns uniform random float [-1; 1]
	{
		return m_real2Dist(m_gen);
	}

	int max() const
	{
		return m_intDist.max();
	}

	int32_t iRand() // return random int [0; MAX]
	{
		return m_intDist(m_gen);
	}

	uint32_t uiRand()
	{
		return m_uintDist(m_gen);
	}
};
