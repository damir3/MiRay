/*
 * Copyright (C) 2014, Petr Vevoda, Martin Sik (http://cgg.mff.cuni.cz/~sik/),
 * Tomas Davidovic (http://www.davidovic.cz), Iliyan Georgiev (http://www.iliyan.com/),
 * Jaroslav Krivanek (http://cgg.mff.cuni.cz/~jaroslav/)
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * (The above is MIT License: http://en.wikipedia.origin/wiki/MIT_License)
 */

#pragma once

class PhaseFunction
{
public:
	static vec3 evaluate(const vec3  &aWorldDirFix, // Points away from the scattering location
						 const vec3  &aWorldDirGen, // Points away from the scattering location
						 const float aMeanCosine,
						 float       *oDirectPdfW = nullptr,
						 float       *oReversePdfW = nullptr,
						 float       *oSinTheta = nullptr)
	{
		float pdf = PhaseFunction::pdf(aWorldDirFix, aWorldDirGen, aMeanCosine, oSinTheta);

		if (oDirectPdfW) *oDirectPdfW = pdf;
		if (oReversePdfW) *oReversePdfW = pdf;

		return vec3(pdf);
	}

	static float pdf(const vec3  &aWorldDirFix, // Points away from the scattering location
					 const vec3  &aWorldDirGen, // Points away from the scattering location
					 const float aMeanCosine,
					 float       *oSinTheta = nullptr)
	{
		assert(aMeanCosine >= -1 && aMeanCosine <= 1);
		assert(isRoughlyNormalized(aWorldDirFix) && isRoughlyNormalized(aWorldDirGen));

		if (isIsotropic(aMeanCosine)) {
			if (oSinTheta) {
				const float cosTheta = -glm::dot(aWorldDirGen, aWorldDirFix);
				*oSinTheta = std::sqrt(std::max(0.f, 1.f - cosTheta * cosTheta));
			}
			return uniformSpherePdfW();
		} else {
			const float cosTheta = -glm::dot(aWorldDirGen, aWorldDirFix);
			const float squareMeanCosine = aMeanCosine * aMeanCosine;
			const float d = 1.f + squareMeanCosine - (aMeanCosine + aMeanCosine) * cosTheta;
			if (oSinTheta)
				*oSinTheta = std::sqrt(std::max(0.f, 1.f - cosTheta * cosTheta));

			return d > 0.f ? ( uniformSpherePdfW() * (1.f - squareMeanCosine) / (d * std::sqrt(d)) ) : 0.f;
		}
	}

	static vec3 sample(const vec3  &aWorldDirFix, // Points away from the scattering location
					   const float aMeanCosine,
					   const vec3  &aRndTriplet,
					   vec3        &oWorldDirGen, // Points away from the scattering location
					   float       &oPdfW,
					   float       *oSinTheta = nullptr)
	{
		Frame frame;
		frame.setFromZ(-aWorldDirFix);
		return sample(aWorldDirFix, aMeanCosine, aRndTriplet, frame, oWorldDirGen, oPdfW, oSinTheta);
	}

	static vec3 sample(const vec3  &aWorldDirFix, // Points away from the scattering location
					   const float aMeanCosine,
					   const vec3  &aRndTriplet,
					   const Frame &aFrame,
					   vec3        &oWorldDirGen, // Points away from the scattering location
					   float       &oPdfW,
					   float       *oSinTheta = nullptr)
	{
		assert(aMeanCosine >= -1 && aMeanCosine <= 1);
		assert(isRoughlyNormalized(aWorldDirFix));

		if (isIsotropic(aMeanCosine)) {
			oWorldDirGen = uniformSampleSphere(aRndTriplet.x, aRndTriplet.y);
			oPdfW = uniformSpherePdfW();
			if (oSinTheta) {
				const float cosTheta = -glm::dot(oWorldDirGen, aWorldDirFix);
				*oSinTheta = std::sqrt(std::max(0.f, 1.f - cosTheta * cosTheta));
			}
		} else {
			const float squareMeanCosine = aMeanCosine * aMeanCosine;
			const float twoCosine = aMeanCosine + aMeanCosine;
			const float sqrtt = (1.f - squareMeanCosine) / (1.f - aMeanCosine + twoCosine * aRndTriplet.x);
			const float cosTheta = (1.f + squareMeanCosine - sqrtt * sqrtt) / twoCosine;
			const float sinTheta = std::sqrt(std::max(0.f, 1.f - cosTheta * cosTheta));
			const float phi = M_2PIf * aRndTriplet.y;
			const float sinPhi = sinf(phi);
			const float cosPhi = cosf(phi);
			const float d = 1.f + squareMeanCosine - twoCosine * cosTheta;

			oWorldDirGen = aFrame.toWorld(vec3(cosPhi * sinTheta, sinPhi * sinTheta, cosTheta));
			oPdfW = d > 0.f ? ( uniformSpherePdfW() * (1.f - squareMeanCosine) / (d * std::sqrt(d)) ) : 0.f;
			if (oSinTheta)
				*oSinTheta = sinTheta;
		}

		assert(isRoughlyNormalized(oWorldDirGen));

		return vec3(oPdfW);
	}
};
