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

#include "PhBeams.h"
#include "KdTmpl.h"
//
//#ifdef USE_BRUTE
//#include "PhBrute.hxx"
//#endif
//
//#ifdef USE_EMBREE
//#include "PhEmbree.hxx"
//#endif

#ifdef USE_GRID
#include "PhGrid.h"
#endif

/**
 * @brief	Defines an alias representing a kd tree for knn queries.
 */
typedef KdTreeTmplPtr< vec3 > KdTree;

const float MAX_FLOAT_SQUARE_ROOT = std::sqrt(std::numeric_limits< float >::max()); //!< The maximum float square root

uint PhotonBeamsEvaluator::sGridSize = 256;     //!< Size of the grid.
uint PhotonBeamsEvaluator::sMaxBeamsInCell = 0; //!< Maximum number of tested beams in a single cell. 0 means no restriction.
uint PhotonBeamsEvaluator::sReductionType = 0;  //!< Type of the reduction of numbers of tested beams in cells.

// ----------------------------------------------------------------------------------------------

/**
 * @brief	Builds the data structure for Beam-Beam queries on the given beams.
 *
 * @param [in,out]	beams	 	The beams.
 * @param	radiusCalculation	Type of radius calculation.
 * @param	beamRadius		 	Beam radius.
 * @param	knn				 	Value x means that x-th closest beam vertex will be used for
 * 								calculation of cone radius at the current beam vertex.
 */
void PhotonBeamsEvaluator::build(PhotonBeamsArray & beams,
								 RadiusCalculation radiusCalculation,
								 const float beamRadius,
								 const int knn) {
	const float SMALLEST_RADIUS = 0.001f;

	assert(accelStruct == nullptr);
	assert( !beams.empty() );

	KdTree * tree = nullptr;
	KdTree::CKNNQuery * query = nullptr;
	if (radiusCalculation == KNN_RADIUS) {
		tree = new KdTree();
		tree->Reserve(beams.size());
		for (int i = 0; i < (int)beams.size(); i++) {
			tree->AddItem(&beams[i].mRay.origin(), i);
		}
		tree->BuildUp();
		query = new KdTree::CKNNQuery(knn);
	}

	// Define radius of each beam
	for (auto it = beams.begin(); it != beams.end(); ++it) {
		if (radiusCalculation == KNN_RADIUS) {
			// Query for beam start
			query->Init(it->mRay.origin(), knn, MAX_FLOAT_SQUARE_ROOT);
			tree->KNNQuery(*query, tree->truePred);
			assert(query->found > 1);
			it->mStartRadius = std::max(2.0f * std::sqrt(query->dist2[1]) * beamRadius,SMALLEST_RADIUS);
			// Query for beam end
			query->Init(it->mRay.target(it->mLength), knn, MAX_FLOAT_SQUARE_ROOT);
			tree->KNNQuery(*query, tree->truePred);
			assert(query->found > 1);
			it->mEndRadius = std::max(2.0f * std::sqrt(query->dist2[1]) * beamRadius, SMALLEST_RADIUS);
			float maxRadius = std::max(it->mStartRadius, it->mEndRadius);
			it->mMaxRadiusSqr = maxRadius * maxRadius;
			it->mRadiusChange = (it->mEndRadius - it->mStartRadius) / it->mLength;
		} else {
			it->mStartRadius = it->mEndRadius = beamRadius;
			it->mMaxRadiusSqr = beamRadius * beamRadius;
			it->mRadiusChange = 0.0f;
		}
		it->mInvocation = 0; // Not yet intersected
	}

	if (radiusCalculation == KNN_RADIUS) {
		delete query;
		delete tree;
	}

	accelStruct = new AccelStruct();
	assert(accelStruct != nullptr);
#ifdef USE_GRID
	accelStruct->setGridSize(sGridSize);
	accelStruct->setMaxBeamsInCell(sMaxBeamsInCell);
	accelStruct->setReductionType(sReductionType);
	accelStruct->setSeed(mSeed);
#endif
	accelStruct->build(beams);
}

// ----------------------------------------------------------------------------------------------

/**
 * @brief	Destroys the data structure for Beam-Beam queries.
 */
void PhotonBeamsEvaluator::destroy() {
	delete accelStruct;
	accelStruct = nullptr;
}

// ----------------------------------------------------------------------------------------------

/**
 * @brief	Evaluates the beam-beam estimate for the given query ray.
 *
 * @param	beamType					   	Type of the beam.
 * @param	queryRay					   	The query ray (=beam) for the Beam-beam estimate.
 * @param	segments					   	Full volume segments of media intersected by the ray.
 * @param	estimatorTechniques			   	The estimator techniques to use.
 * @param	raySamplingFlags			   	The ray sampling flags (\c kOriginInMedium).
 * @param [in,out]	additionalRayDataForMis	(Optional) additional data needed for MIS weights
 * 											computations.
 * @param [in,out]	gridStats			   	(Optional) statistics to gather for the ray.
 *
 * @return	The accumulated radiance along the ray.
 */
vec3 PhotonBeamsEvaluator::evalBeamBeamEstimate(BeamType beamType,
												const Ray& queryRay,
												const VolumeSegments& segments,
												const uint estimatorTechniques,
												const uint raySamplingFlags,
												AdditionalRayDataForMis* additionalRayDataForMis,
												GridStats* gridStats) {
	assert(estimatorTechniques & BB1D);
	assert(raySamplingFlags == 0 || raySamplingFlags == kOriginInMedium);

	vec3 result(0.f);

	vec3 attenuation(1.f);
	float raySamplePdf = 1.0f;
	float raySampleRevPdf = 1.0f;
	GridStats _gridStats;
	if (!gridStats) gridStats = &_gridStats;

	/// Accumulate for each segment
	for (auto it = segments.begin(); it != segments.end(); ++it) {
		// Get segment medium
		auto medium = it->mMedium;

		// Accumulate
		vec3 segmentResult(0.f);
		if (medium->hasScattering()) {
			if (additionalRayDataForMis) {
				additionalRayDataForMis->mRaySamplePdf = raySamplePdf;
				additionalRayDataForMis->mRaySampleRevPdf = raySampleRevPdf;
				additionalRayDataForMis->mRaySamplingFlags = kEndInMedium;
				if (it == segments.begin())
					additionalRayDataForMis->mRaySamplingFlags |= raySamplingFlags;
			}
			segmentResult = accelStruct->evalBeamBeamEstimate(queryRay, beamType | estimatorTechniques, medium, it->mDistMin, it->mDistMax, *gridStats, additionalRayDataForMis);
		}
		// Add to total result
		result += attenuation * segmentResult;

//		if (additionalRayDataForMis)
//		{
//			DebugImages & debugImages = *static_cast<DebugImages *>(additionalRayDataForMis->mDebugImages);
//			debugImages.accumRgb2ToRgb(DebugImages::BB1D, attenuation);
//			debugImages.ResetAccum2();
//		}

		// Update attenuation
		attenuation *= beamType == SHORT_BEAM ? it->mAttenuation / it->mRaySamplePdf :  // Short beams - no attenuation
			it->mAttenuation;
		if (!isPositive(attenuation))
			return result;

		// Update PDFs
		raySamplePdf *= it->mRaySamplePdf;
		raySampleRevPdf *= it->mRaySampleRevPdf;
	}

	assert(!isNanInfNeg(result));

	return result;
}

/**
 * @brief	Evaluates the beam-beam estimate for the given query ray.
 *
 * @param	beamType					   	Type of the beam.
 * @param	queryRay					   	The query ray (=beam) for the Beam-beam estimate.
 * @param	segments					   	Lite volume segments of media intersected by the ray.
 * @param	estimatorTechniques			   	The estimator techniques to use.
 * @param	raySamplingFlags			   	The ray sampling flags (\c kOriginInMedium).
 * @param [in,out]	additionalRayDataForMis	(Optional) additional data needed for MIS weights
 * 											computations.
 * @param [in,out]	gridStats			   	(Optional) statistics to gather for the ray.
 *
 * @return	The accumulated radiance along the ray.
 */
//vec3 PhotonBeamsEvaluator::evalBeamBeamEstimate(BeamType beamType,
//												const Ray& queryRay,
//												const LiteVolumeSegments& segments,
//												const uint estimatorTechniques,
//												const uint raySamplingFlags,
//												AdditionalRayDataForMis* additionalRayDataForMis,
//												GridStats* gridStats)
//{
//	assert(beamType == LONG_BEAM);
//	assert(estimatorTechniques & BB1D);
//	assert(raySamplingFlags == 0 || raySamplingFlags == kOriginInMedium);
//
//	vec3 result(0.f);
//
//	vec3 attenuation(1.f);
//	float raySamplePdf = 1.0f;
//	float raySampleRevPdf = 1.0f;
//	GridStats _gridStats;
//	if (!gridStats) gridStats = &_gridStats;
//
//	/// Accumulate for each segment
//	for (auto it = segments.begin(); it != segments.end(); ++it)
//	{
//		// Get segment medium
//		const MaterialImpl * medium = scene.mMedia[it->mMediumID];
//
//		// Accumulate
//		vec3 segmentResult(0.f);
//		if (medium->HasScattering())
//		{
//			if (additionalRayDataForMis)
//			{
//				additionalRayDataForMis->mRaySamplePdf = raySamplePdf;
//				additionalRayDataForMis->mRaySampleRevPdf = raySampleRevPdf;
//				additionalRayDataForMis->mRaySamplingFlags = kEndInMedium;
//				if (it == segments.begin())
//					additionalRayDataForMis->mRaySamplingFlags |= raySamplingFlags;
//			}
//			segmentResult = accelStruct->evalBeamBeamEstimate(queryRay, beamType | estimatorTechniques, medium, it->mDistMin, it->mDistMax, *gridStats, additionalRayDataForMis);
//		}
//
//		// Add to total result
//		result += attenuation * segmentResult;
//
//		if (additionalRayDataForMis)
//		{
//			DebugImages & debugImages = *static_cast<DebugImages *>(additionalRayDataForMis->mDebugImages);
//			debugImages.accumRgb2ToRgb(DebugImages::BB1D, attenuation);
//			debugImages.ResetAccum2();
//		}
//
//		// Update attenuation
//		attenuation *= medium->EvalAttenuation(queryRay, it->mDistMin, it->mDistMax);
//		if (!isPositive(attenuation))
//			return result;
//
//		// Update PDFs
//		float segmentRaySampleRevPdf;
//		float segmentRaySamplePdf = medium->RaySamplePdf(queryRay, it->mDistMin, it->mDistMax, it == segments.begin() ? raySamplingFlags : 0, &segmentRaySampleRevPdf);
//		raySamplePdf *= segmentRaySamplePdf;
//		raySampleRevPdf *= segmentRaySampleRevPdf;
//	}
//
//	assert(!isNanInfNeg(result));
//
//	return result;
//}

// ----------------------------------------------------------------------------------------------

/**
 * @brief	Gets probability of selecting a beam (in case of beam reduction) around the given
 * 			position.
 *
 * @param	pos	The position.
 *
 * @return	The beam selection PDF.
 */
float PhotonBeamsEvaluator::getBeamSelectionPdf(const vec3 & pos) const
{
	return accelStruct->getBeamSelectionPdf(pos);
}
