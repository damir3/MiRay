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

/*
 Based on https://github.com/PetrVevoda/smallupbp/blob/master/SmallUPBP/src/Renderers/VolPathTracer.hxx
 Modified by Damir Sagidullin
 */

#include "../Materials/MaterialImpl.h"
#include "../Materials/BSDF.h"
#include "VolumePathTracer.h"

// ------------------------------------------------------------------------ //

VolumePathTracer::VolumePathTracer(Scene & scene)
	: BaseRenderer(scene)
{
//#ifdef _DEBUG
//	m_numCPU = 1;
//#endif
}

VolumePathTracer::~VolumePathTracer()
{
}

// ------------------------------------------------------------------------ //

// Mis power (1 for balance heuristic)
inline float mis(float aPdf)
{
	return aPdf;
}

// Mis weight for 2 pdfs
inline float mis2(float aSamplePdf, float aOtherPdf)
{
	return mis(aSamplePdf) / (mis(aSamplePdf) + mis(aOtherPdf));
}

// ------------------------------------------------------------------------ //

void VolumePathTracer::traceCameraPath(TraceResult & res, TraceContext & ctx)
{
	Isect	isect(m_rayLength);
	vec3 	pathWeight(1.f);
	bool	lastSpecular = true;
	float	lastPdfW = 1.f;
	int		specularPath = 1;
	bool	originInMedium = false;

	VolumeSegments	mVolumeSegments;
	BoundaryStack	mBoundaryStack;
	initBoundaryStack(mBoundaryStack);

	Ray & ray = ctx.ray;
	ray.boundaryStack = &mBoundaryStack;

	for (uint pathLength = 0; ; ++pathLength) {
		mVolumeSegments.clear();
		if (!intersect(ray, isect, ctx.sampler, mBoundaryStack, kSampleVolumeScattering, originInMedium ? kOriginInMedium : 0, &mVolumeSegments)) {
			// In attenuating media the ray can never travel to infinity
			//assert(mScene.GetGlobalMediumPtr()->GetAttenuationCoef(ray.origin).isBlackOrNegative());
			if (isect.mFloor) {
				if (ray.direction().z < 0.f && ray.origin().z > m_floor.level) {
					auto opacity = evaluateShadowOpacity(res, ctx, mBoundaryStack, mVolumeSegments);
					if (pathLength == 0) {
						res.opacity = vec3(opacity);
						res.normal = vec3(0.f, 0.f, 1.f);
						res.depth = -ctx.ray.origin().z / ctx.ray.direction().z;
						break;
					}

					pathWeight *= 1.f - opacity;
				}
			}

			if (ctx.floor) { // restore ray state for environment lighting
				ray.setOrigin(ctx.floorRayOrigin);
				ray.setDirection(ctx.floorRayDirection);
			}

			// At the moment we do not support background illumination of scenes with emissive global medium
			//if (background && !mScene.GetGlobalMediumPtr()->GetEmissionCoef(ray.origin).isBlackOrNegative())
			//	break;

			// Compute emission from intersected media (if any)
			if (!mVolumeSegments.empty() && (pathLength == 0 || lastSpecular))
				res.addColor(pathWeight * VolumeSegment::AccumulateAttenuatedEmissionWithPdf(mVolumeSegments));

//			// In attenuating media the ray can never travel to infinity
//			if (mScene.GetGlobalMediumPtr()->HasAttenuation())
//				break;

			// Attenuate by intersected media (if any)
			float raySamplePdf(1.f);
			if (!mVolumeSegments.empty()) {
				// PDF
				raySamplePdf = VolumeSegment::AccumulatePdf(mVolumeSegments);
				assert(raySamplePdf > 0);

				// Attenuation
				pathWeight *= VolumeSegment::AccumulateAttenuationWithoutPdf(mVolumeSegments) / raySamplePdf;
			}

			if (isBlack(pathWeight))
				break;

			// Compute contribution
			if (!ctx.reflected && m_bgMode != BackgroundMode_Environment) {
				res.opacity = vec3(1.f) - pathWeight;
				break;
			}

			for (const auto * light : m_infiniteLights) {
				// For background we cheat with the A/W suffixes,
				// and GetRadiance actually returns W instead of A
				float directPdfW;
				auto contrib = light->getRadiance(ray, &directPdfW);
				if (isBlack(contrib))
					continue;

				// Compute MIS weight (if in MIS mode and we could have sampled this light last time in the next event estimation)
				float misWeight = 1.f;
				if (pathLength > 0 && !lastSpecular) {
					misWeight = mis2(lastPdfW * raySamplePdf, directPdfW * m_lightPickProbability);
				}

				// Add attenuated contribution
				res.addColor(pathWeight * misWeight * contrib);
			}

			// We have left the scene
			break;
		}

		assert(isect.isValid());

		if (pathLength == 0) {
			res.normal = ctx.ray.normal;
			res.depth = ctx.ray.ray.tfar;
			res.node = ctx.ray.node;
			res.geometry = ctx.ray.geom;
		}

		// Attenuate by intersected media (if any)
		float raySamplePdf(1.f);
		if (!mVolumeSegments.empty()) {
			// Emission
			// If in light sampling mode be careful not to create path which hits a light, adds emission on the path segment hitting it, but
			// not its emitted radiance. Such path cannot be created by direct path tracing.
			res.addColor(pathWeight * VolumeSegment::AccumulateAttenuatedEmissionWithPdf(mVolumeSegments));

			// PDF
			raySamplePdf = VolumeSegment::AccumulatePdf(mVolumeSegments);
			assert(raySamplePdf > 0);

			// Attenuation
			pathWeight *= VolumeSegment::AccumulateAttenuationWithoutPdf(mVolumeSegments) / raySamplePdf;
		}

		if (isBlack(pathWeight))
			break;

		// Directly hit some light
		if (isect.mLight) {
			// Compute its contribution
			float cosThetaFix;
			float directIllumPdfA;
			auto contrib = isect.mLight->getRadiance(ray, &directIllumPdfA, nullptr, &cosThetaFix);

			if (isBlack(contrib))
				break;

			// Compute MIS weight (if in MIS mode and we could have sampled this light last time in the next event estimation)
			float misWeight = 1.f;
			if (pathLength > 0 && !lastSpecular) {
				const float directIllumPdfW = pdfAtoW(directIllumPdfA, isect.mDist, cosThetaFix);
				misWeight = mis2(lastPdfW * raySamplePdf, directIllumPdfW * m_lightPickProbability);
			}

			// Add attenuated contribution
			res.addColor(pathWeight * misWeight * contrib);

			// Lights do not reflect
			break;
		}

		BSDF bsdf;
		if (isect.mFloor) {
			if (!bsdf.setupFloor(ctx, isect, 1.f, m_floor, false))
				break;

			// save ray state for environment lighting
			ctx.floorRayOrigin = ray.origin();
			ctx.floorRayDirection = ray.direction();
		} else {
			if (!bsdf.setup(ctx, isect, mBoundaryStack, false))
				break;
		}

		if (pathLength == 0)
			res.albedo = bsdf.albedo();

		if (isect.isOnSurface() && !isBlack(bsdf.emission())) {
			assert(isect.mGeometry);

			float misWeight = 1.f;
			if (pathLength > 0 && !lastSpecular && isect.mGeometry->isLightSource()) { // weight using the balance heuristic
				float lightPdfA;
				isect.mGeometry->getRadiance(ctx.ray, &lightPdfA, nullptr, nullptr);

				const float lightPdfW = pdfAtoW(lightPdfA, ctx.ray.ray.tfar, bsdf.cosThetaFix());
				misWeight = mis2(lastPdfW, lightPdfW * m_lightPickProbability);
			}

			res.addColor(pathWeight * misWeight * bsdf.emission());
		}

		// Break if already at maximum path length
		if (pathLength >= m_maxPathLength)
			break;

		// Get continuation probability
		const float contProb = bsdf.continuationProbability();

		// Next event estimation (if not in direct mode, not on a delta material, path is not too short for end and there are lights to sample)
		if (!bsdf.isDelta() && m_lightCount > 0) {
			// Pick light
			auto light = getLight(size_t(ctx.sampler.generate1D() * m_lightCount));
			assert(light);

			// Light in infinity in attenuating homogeneous global medium is always reduced to zero, while emission along infinite ray is infinite
//			if (light->isFinite())
			{
				// Sample light
				vec3 directionToLight;
				float distanceToLight, directIllumPdfW;
				vec3 radiance = light->illuminate(ctx.ray.hitPoint, ctx.sampler, directionToLight, distanceToLight, directIllumPdfW);

				if (!isBlack(radiance)) {
					// Compute attenuation from scattering function
					float cosThetaOut, scatterPdfW;
					vec3 scatterFactor = bsdf.evaluate(directionToLight, cosThetaOut, &scatterPdfW);

					if (!isBlack(scatterFactor)) {
						// Test occlusion
						mVolumeSegments.clear();
						if (!occluded(ctx.ray.hitPoint, directionToLight, distanceToLight, ctx.ray, ctx.sampler,
									  mBoundaryStack, isect.isInMedium() ? kOriginInMedium : 0, mVolumeSegments)) {
							// Get attenuation from intersected media (if any)
							float nextRaySamplePdf(1.0f);
							vec3 nextAttenuation(1.0f);
							vec3 nextEmission(0.0f);
							if (!mVolumeSegments.empty()) {
								// Emission (without PDF!)
								nextEmission = VolumeSegment::AccumulateAttenuatedEmissionWithoutPdf(mVolumeSegments);

								// PDF
								nextRaySamplePdf = VolumeSegment::AccumulatePdf(mVolumeSegments);
								assert(nextRaySamplePdf > 0);

								// Attenuation (without PDF!)
								nextAttenuation = VolumeSegment::AccumulateAttenuationWithoutPdf(mVolumeSegments);
							}

							// Compute MIS weight (if in MIS mode and it is possible to hit the light directly)
							float misWeight = 1.f;
							if (!light->isDelta()) {
//								scatterPdfW *= contProb * nextRaySamplePdf;
								scatterPdfW *= nextRaySamplePdf;
								misWeight = mis2(directIllumPdfW * m_lightPickProbability, scatterPdfW);
							}

							// Compute contribution
							vec3 contrib = (radiance * nextAttenuation + nextEmission) * scatterFactor * cosThetaOut / (m_lightPickProbability * directIllumPdfW);

							// Add attenuated contribution
							res.addColor(pathWeight * misWeight * contrib);
						}
					}
				}
			}
		}

		// Continue random walk

		if (contProb == 0)
			break;

		// Scattering function sampling
		auto rndTriplet = ctx.sampler.generate3D();
		vec3 dirGen;
		vec3 scatterFactor;
		float scatterPdf, cosThetaOut;
		uint32_t sampledEvent;

		scatterFactor = bsdf.sample(ctx, rndTriplet, dirGen,
									scatterPdf, cosThetaOut,
									&sampledEvent);

		if (isBlack(scatterFactor) || scatterPdf <= 0.f)
			break;

		if (!isfinite(scatterPdf)) {
			LogError() << QString().asprintf("BSDF sample error: rnd(%f,%f,%f) sf=(%f,%f,%f) pdf=%f cto=%f se=%d",
				   rndTriplet.x, rndTriplet.y, rndTriplet.z, scatterFactor.x, scatterFactor.y, scatterFactor.z, scatterPdf, cosThetaOut, sampledEvent);
			break;
		}

		// Russian roulette
		if (pathLength > m_russianRouletteStart && contProb < 1.f) { // Russian roulette
			if (ctx.sampler.generate1D() > contProb)
				break;

			scatterPdf *= contProb;
		}

		lastPdfW = scatterPdf;

		pathWeight *= scatterFactor * (cosThetaOut / scatterPdf);

		if (isBlack(pathWeight))
			break;

		// Update state according to the scattering function type
		if (isect.isOnSurface()) {
			lastSpecular = (sampledEvent & BSDF::kSpecular) != 0;
			originInMedium = false;

			// Switch medium on refraction
			if ((sampledEvent & (BSDF::kRefract | BSDF::kTransmit)) != 0)
				updateBoundaryStackOnRefract(isect, mBoundaryStack);
		} else {
			lastSpecular = false;
			originInMedium = true;
		}

		specularPath &= lastSpecular ? 1 : 0;

		// Update ray
		ray.setOrigin(ray.hitPoint);
		ray.setDirection(dirGen);
		ray.ray.tnear = 0.f;
		ray.ray.tfar = m_rayLength;

		isect.mDist = m_rayLength;
	}
}
