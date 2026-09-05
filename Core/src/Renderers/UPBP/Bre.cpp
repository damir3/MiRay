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

// embree includes

#include "Bre.h"
#include "PhBeams.h"
#include "KdTmpl.h"
#include "PathWeight.h"

/**
 * @brief	Defines an alias representing the kd tree.
 */
typedef KdTreeTmplPtr< vec3 > KdTree;

const float MAX_FLOAT_SQUARE_ROOT = std::sqrt(std::numeric_limits< float >::max());	//!< The maximum float square root

// ------------------------------------------------------------------------ //

struct EmbreeRay : public RTCRayHit
{
	const MaterialImpl *	medium;					//!< Medium that this ray passes through (used for BRE and photon beams)
	vec3 *					accumResult;			//!< Result radiance calculation alongthe ray (in BRE and photon beams), type Rgb*
	uint					flags;					//!< Additional flags
	const Ray *				originalRay;			//!< Original ray
	const AdditionalRayDataForMis * additionalRayDataForMis;	//!< Additional data needed for MIS weights computation

	EmbreeRay() {}

	EmbreeRay(const vec3 & origin, const vec3 & direction, float tmin, float tmax)
	{
		ray.org_x = origin.x;
		ray.org_y = origin.y;
		ray.org_z = origin.z;
		ray.dir_x = direction.x;
		ray.dir_y = direction.y;
		ray.dir_z = direction.z;
		ray.tnear = tmin;
		ray.tfar = tmax;
		hit.geomID = RTC_INVALID_GEOMETRY_ID;
		hit.primID = RTC_INVALID_GEOMETRY_ID;
		hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
		ray.mask = 0xFFFFFFFF;
		ray.time = 0.f;
	}

	void SetAdditionalData(const MaterialImpl* aMedium,
						   vec3 * aAccumResult,
						   uint aFlags,
						   const Ray * aOriginalRay,
						   const AdditionalRayDataForMis * aAdditionalRayDataForMis = nullptr)
	{
		medium = aMedium;
		accumResult = aAccumResult;
		flags = aFlags;
		originalRay = aOriginalRay;
		additionalRayDataForMis = aAdditionalRayDataForMis;
	}
};

// ------------------------------------------------------------------------ //

/**
 * @brief	An embree photon.
 */
class EmbreePhoton
{
public:
	float	radius;    //!< The radius.
	float	radiusSqr; //!< The radius squared.
//	vec3	incDir;    //!< The incoming direction.
//	vec3	flux;	  //!< The flux.
	const UPBPLightVertex *lightVertex; //!< The corresponding light vertex.

	/**
	 * @brief	Sets photon properties and function pointers.
	 *
	 * @param	aRadius			The radius.
	 * @param	aLightVertex	The light vertex corresponding to the photon.
	 */
	void set(const float aRadius, const UPBPLightVertex * aLightVertex)
	{
		assert(isfinite(aRadius));
		assert(aRadius > 0.f);
		assert(isfinite(aLightVertex->mHitpoint.x) && isfinite(aLightVertex->mHitpoint.y) && isfinite(aLightVertex->mHitpoint.z));

		radius = aRadius;
		radiusSqr = aRadius*aRadius;
		lightVertex = aLightVertex;
//		incDir = aLightVertex->mBSDF.worldDirFix();
//		flux = aLightVertex->mThroughput;
	}

	/**
	 * @brief	Test intersection between a ray and a 'photon disc'.
	 *
	 * 			The photon disc is specified by the position aPhotonPos and radius aPhotonRad. The
	 * 			disc is assumed to face the ray (i.e. the disc plane is perpendicular to the ray).
	 * 			Intersections are reported only in the interval [aMinT, aMaxT)  (i.e. includes aMinT
	 * 			but excludes aMaxT).
	 *
	 * @param [in,out]	oIsectDist  	The intersection distance along the ray.
	 * @param [in,out]	oIsectRadSqr	The square of the distance of the intersection point from the photon location.
	 * @param	aQueryRay				The query ray.
	 * @param	aMinT					The minimum t.
	 * @param	aMaxT					The maximum t.
	 * @param	aPhotonPos				The photon position.
	 * @param	aPhotonRadSqr			The photon radius.
	 *
	 * @return	true is an intersection is found, false otherwise. If an intersection is found,
	 * 			oIsectDist is set to the intersection distance along the ray, and oIsectRadSqr is set
	 * 			to the square of the distance of the intersection point from the photon location.
	 */
	static bool TestIntersectionBre(float& oIsectDist, float& oIsectRadSqr,
									const Ray &aQueryRay,
									const float aMinT, const float aMaxT,
									const vec3 &aPhotonPos, const float aPhotonRadSqr)
	{
		const vec3 rayOrigToPhoton = aPhotonPos - aQueryRay.origin();
		const float isectDist = glm::dot(rayOrigToPhoton, aQueryRay.direction());

		if (isectDist > aMinT && isectDist < aMaxT) {
			const float isectRadSqr = glm::length2(aQueryRay.target(isectDist) - aPhotonPos);
			if (isectRadSqr <= aPhotonRadSqr) {
				oIsectDist   = isectDist;
				oIsectRadSqr = isectRadSqr;
				return true;
			}
		}

		return false;
	}

	static void BoundsFunc(const EmbreePhoton* photons, size_t item, RTCBounds & bounds)
	{
		auto & photon = photons[item];
		auto & pos = photon.lightVertex->mHitpoint;
		bounds.lower_x = pos.x - photon.radius;
		bounds.lower_y = pos.y - photon.radius;
		bounds.lower_z = pos.z - photon.radius;
		bounds.upper_x = pos.x + photon.radius;
		bounds.upper_y = pos.y + photon.radius;
		bounds.upper_z = pos.z + photon.radius;
	}

	/**
	 * @brief	BRE intersection function for a photon.
	 *
	 * 			Version used by combined PB2D algorithms from \c UPBP.hxx.
	 *
	 * @param	This	   	EmbreePhoton that we are testing intersection with.
	 * @param [in,out]	ray	BRE query ray.
	 *
	 * @return	If no intersection is found, ray is left unchanged. To report an intersection
	 * 			back to embree, one needs to set ray.tfar to the intersection distance and
	 * 			ray.id0 and ray.id1 to the id of the intersected object. In the BRE query we
	 * 			never report intersections to embree because we want to keep traversing the data
	 * 			structure.
	 */
	static void IntersectFunc(const EmbreePhoton* photons, EmbreeRay& ray, size_t item)
	{
		auto & photon = photons[item];
		auto lightVertex = photon.lightVertex;
		auto data = ray.additionalRayDataForMis;

		assert(lightVertex);
		assert(lightVertex->mInMedium);
		assert(data);

		float photonIsectDist, isectRadSqr;

		/*if (TestIntersectionBre(photonIsectDist, isectRadSqr, *ray.originalRay, ray.tnear, ray.tfar, photon.pos, photon.radiusSqr))
		{
			// Found an intersection.
			const vec3 isectPt = ray.originalRay->Target(photonIsectDist);

			const vec3& scatteringCoeff = ray.medium->GetScatteringCoef();

			vec3 attenuation;
//			if (ray.medium->isHomogeneous())
			{
				auto medium = ray.medium;
				attenuation = medium->EvalAttenuation(photonIsectDist - ray.tnear);
				if (ray.flags & SHORT_BEAM)
					attenuation /= attenuation[medium->mMinPositiveAttenuationCoefCompIndex()];
			}
//			else
//			{
//				attenuation = ray.medium->EvalAttenuation(*ray.origRay, ray.tnear, photonIsectDist);
//				if (ray.flags & SHORT_BEAM)
//					attenuation /= ray.medium->RaySamplePdf(*ray.origRay, ray.tnear, photonIsectDist);
//			}

			*ray.accumResult += photon.flux * attenuation * scatteringCoeff *
				PhaseFunction::evaluate(ray.originalRay->direction(), -photon.incDir, ray.medium->MeanCosine()) *
				// Epanechnikov kernel
				(1 - isectRadSqr / photon.radiusSqr) / (photon.radiusSqr * M_HALF_PIf);

			assert(!isNanInfNeg(*ray.accumResult));

//			assert((1 - isectRadSqr) / (M_PIf * (photon.radiusSqr - photon.radiusSqr * photon.radiusSqr * 0.5f)) > 0.0f);
		}*/

		if (TestIntersectionBre(photonIsectDist, isectRadSqr, *ray.originalRay, ray.ray.tnear, ray.ray.tfar, lightVertex->mHitpoint, photon.radiusSqr)) {
			assert(photonIsectDist);

			// Heterogeneous medium is not supported.
			assert(ray.medium->isHomogeneous());

			// Reject if full path length below/above min/max path length.
			if ((lightVertex->mPathLength + data->mCameraPathLength > data->mMaxPathLength) ||
				(lightVertex->mPathLength + data->mCameraPathLength < data->mMinPathLength))
				return;

			// Ignore contribution of primary rays from medium too close to camera.
			if (data->mCameraPathLength == 1 && photonIsectDist < data->mMinDistToMed)
				return;

			// Compute intersection.
			const vec3 isectPt = ray.originalRay->target(photonIsectDist);

			// Compute attenuation in current segment and overall pdfs.
			vec3 attenuation;
			float raySamplePdf = 1.0f;
			float raySampleRevPdf = 1.0f;
			float raySamplePdfsRatio = 1.0f;
			assert(ray.medium->isHomogeneous());
//			if (ray.medium->isHomogeneous())
			{
				auto medium = ray.medium;
				attenuation = medium->evalAttenuation(photonIsectDist - ray.ray.tnear);
				const float pdf = attenuation[medium->mMinPositiveAttenuationCoefCompIndex()];
				if (ray.flags & SHORT_BEAM)
					attenuation /= pdf;
				raySamplePdf = medium->mMinPositiveAttenuationCoefComp() * pdf;
				raySampleRevPdf = (data->mRaySamplingFlags & kOriginInMedium) ? raySamplePdf : pdf;
				raySamplePdfsRatio = 1.0f / medium->mMinPositiveAttenuationCoefComp();
			}
//			else
//			{
//				attenuation = ray.medium->EvalAttenuation(*ray.originalRay, ray.ray.tnear, photonIsectDist);
//				if (ray.flags & SHORT_BEAM)
//					attenuation /= ray.medium->RaySamplePdf(*ray.originalRay, ray.ray.tnear, photonIsectDist);
//				raySamplePdf = ray.medium->RaySamplePdf(*ray.originalRay, ray.ray.tnear, photonIsectDist, data->mRaySamplingFlags, &raySampleRevPdf);
//				raySamplePdfsRatio = ray.medium->RaySamplePdf(*ray.originalRay, ray.ray.tnear, photonIsectDist, 0) / raySamplePdf;
//			}
			if (!isPositive(attenuation))
				return;

			raySamplePdf *= data->mRaySamplePdf;
			raySampleRevPdf *= data->mRaySampleRevPdf;
			assert(raySamplePdf);
			assert(raySampleRevPdf);

			// Retrieve light incoming direction in world coordinates.
			const vec3 lightDirection = lightVertex->mBSDF.worldDirFix();

			// BSDF.
			float cameraBsdfDirPdfW, cameraBsdfRevPdfW, sinTheta;
			const vec3 cameraBsdfFactor = PhaseFunction::evaluate(-(*(vec3 *)&ray.ray.dir_x), lightDirection, ray.medium->meanCosine(), &cameraBsdfDirPdfW, &cameraBsdfRevPdfW, &sinTheta);
			if (isBlack(cameraBsdfFactor))
				return;

			cameraBsdfDirPdfW *= ray.medium->continuationProbability();
			assert(cameraBsdfDirPdfW > 0);

			// Even though this is PDF from camera BSDF, the continuation probability
			// must come from light BSDF, because that would govern it if light path
			// actually continued.
			cameraBsdfRevPdfW *= lightVertex->mBSDF.continuationProbability();
			assert(cameraBsdfRevPdfW > 0);

			// Epanechnikov kernel.
			const float kernel = (1 - isectRadSqr / photon.radiusSqr) / (photon.radiusSqr * M_PIf * 0.5f);
			if (!isPositive(kernel))
				return;

			// Scattering coefficient.
//			const vec3 & scatteringCoeff = ray.medium->isHomogeneous() ? ((const HomogeneousMedium *)ray.medium)->GetScatteringCoef() : ray.medium->GetScatteringCoef(isectPt);
			const vec3 & scatteringCoeff = ray.medium->getScatteringCoef();

			// Unweighted result.
			const vec3 unweightedResult = lightVertex->mThroughput * attenuation * scatteringCoeff * cameraBsdfFactor * kernel;

			if (isBlack(unweightedResult))
				return;

			// Update affected MIS data.
			const float distSq = sqr(photonIsectDist);
			const float raySamplePdfInv = 1.0f / raySamplePdf;
			MisData* cameraVerticesMisData = data->mCameraVerticesMisData;
			cameraVerticesMisData[data->mCameraPathLength].mPdfAInv = data->mLastPdfWInv * distSq * raySamplePdfInv;
			//cameraVerticesMisData[data->mCameraPathLength].mRevPdfA = 1.0f; // not used (sent through accumulateCameraPathWeight params)
			cameraVerticesMisData[data->mCameraPathLength].mRaySamplePdfInv = raySamplePdfInv;
			//cameraVerticesMisData[data->mCameraPathLength].mRaySampleRevPdfInv = lightVertex->mMisData.mRaySamplePdfInv; // not used (sent through accumulateCameraPathWeight params)
			cameraVerticesMisData[data->mCameraPathLength].mRaySamplePdfsRatio = raySamplePdfsRatio;
			//cameraVerticesMisData[data->mCameraPathLength].mRaySampleRevPdfsRatio = lightVertex->mMisData.mRaySamplePdfsRatio; // not used (sent through accumulateCameraPathWeight params)
			//cameraVerticesMisData[data->mCameraPathLength].mSinTheta = sinTheta; // not used (sent through accumulateCameraPathWeight params)
			cameraVerticesMisData[data->mCameraPathLength].mSurfMisWeightFactor = 0;
			cameraVerticesMisData[data->mCameraPathLength].mPP3DMisWeightFactor = data->mPP3DMisWeightFactor;
			cameraVerticesMisData[data->mCameraPathLength].mPB2DMisWeightFactor = data->mPB2DMisWeightFactor; //data->mLightSubPathCount / kernel;
			cameraVerticesMisData[data->mCameraPathLength].mBB1DMisWeightFactor = data->mBB1DMisWeightFactor;
			if (!data->mBB1DPhotonBeams)
				cameraVerticesMisData[data->mCameraPathLength].mBB1DBeamSelectionPdf = 0.0f;
			else
			{
				auto pbe = data->mBB1DPhotonBeams;
				if (pbe->sMaxBeamsInCell)
					cameraVerticesMisData[data->mCameraPathLength].mBB1DBeamSelectionPdf = pbe->getBeamSelectionPdf(isectPt);
				else
					cameraVerticesMisData[data->mCameraPathLength].mBB1DBeamSelectionPdf = 1.0f;
			}
			cameraVerticesMisData[data->mCameraPathLength].mIsDelta = false;
			cameraVerticesMisData[data->mCameraPathLength].mIsOnLightSource = false;
			cameraVerticesMisData[data->mCameraPathLength].mIsSpecular = false;
			cameraVerticesMisData[data->mCameraPathLength].mInMediumWithBeams = ray.medium->getMeanFreePath(isectPt) > data->mBB1DMinMFP;

			// Update reverse PDFs of the previous vertex.
			cameraVerticesMisData[data->mCameraPathLength - 1].mRaySampleRevPdfInv = 1.0f / raySampleRevPdf;

			// Compute MIS weight.
			const float last = (ray.flags & SHORT_BEAM) ?
				1.0 / (raySamplePdfsRatio * cameraVerticesMisData[data->mCameraPathLength].mPB2DMisWeightFactor) :
				raySamplePdf / cameraVerticesMisData[data->mCameraPathLength].mPB2DMisWeightFactor;
			const float wCamera = accumulateCameraPathWeight(data->mCameraPathLength,
															 last,
															 sinTheta,
															 lightVertex->mMisData.mRaySamplePdfInv,
															 lightVertex->mMisData.mRaySamplePdfsRatio,
															 cameraBsdfRevPdfW * raySampleRevPdf / distSq,
															 data->mQueryBeamType,
															 data->mPhotonBeamType,
															 ray.flags,
															 cameraVerticesMisData);
			const float wLight = AccumulateLightPathWeight(data->mLightVertices + data->mPathSegments[lightVertex->mPathIdx].begin,
														   lightVertex->mPathLength,
														   last,
														   0,
														   0,
														   0,
														   cameraBsdfDirPdfW,
														   PB2D,
														   data->mQueryBeamType,
														   data->mPhotonBeamType,
														   ray.flags,
														   false);
			const float misWeight = 1.f / (wLight + wCamera);

			// Weight and accumulate contribution.
			*ray.accumResult += misWeight * unweightedResult;
		}
	}

	/**
	 * @brief	BRE intersection function for a photon - should never be called.
	 *
	 * @param	This	   	This.
	 * @param [in,out]	ray	The ray.
	 *
	 * @return	Never returns.
	 */
	static void OccludedFunc(void* ptr, RTCRay& ray, size_t item)
	{
		assert(!"Error: EmbreePhoton::OccludedFunc() called - makes not sense");
	}
};

// ------------------------------------------------------------------------ //

/**
 * @brief	Build a structure of 'virtual' objects - i.e. photon spheres.
 *
 * @param [in,out]	photons	The photons.
 * @param	numPhotons	   	Number of photons.
 *
 * @return	The structure of photon spheres.
 */
static RTCScene BuildPhotonTree(RTCDevice device, EmbreePhoton *photons, int numPhotons)
{
//	auto scene = rtcDeviceNewScene(device, RTC_SCENE_STATIC, RTC_INTERSECT1);
//	assert(scene);
//
//	auto geomID = rtcNewUserGeometry(scene, numPhotons);
//
//	rtcSetUserData(scene, geomID, photons);
//	rtcSetBoundsFunction(scene, geomID, (RTCBoundsFunc)&EmbreePhoton::BoundsFunc);
//	rtcSetIntersectFunction(scene, geomID, (RTCIntersectFunc)&EmbreePhoton::IntersectFunc);
//	rtcSetOccludedFunction(scene, geomID, &EmbreePhoton::OccludedFunc);
//
//	rtcCommit(scene);
//    return scene;
	return nullptr;
}

// ------------------------------------------------------------------------ //

/**
 * @brief	Build the data structure for BRE queries.
 *
 * 			Version used by combined PB2D algorithms from \c UPBP.hxx.
 *
 * @param	lightSubPathVertices	Light sub path vertices.
 * @param	numVertices				Number of vertices.
 * @param	radiusCalculation   	Type of radius calculation.
 * @param	photonRadius			Photon radius.
 * @param	knn						Value x means that x-th closest photon will be used for
 * 									calculation of radius of the current photon.
 *
 * @return	Number of photons made from the given light sub path vertices.
 */
int EmbreeBre::Build(RTCDevice device, const UPBPLightVertex* lightSubPathVertices, const int numVertices, RadiusCalculation radiusCalculation, const float photonRadius,
	const int knn)
{
	assert(embreePhotons == nullptr);
	assert(numEmbreePhotons == 0);
	assert(m_rtcScene == nullptr);
	assert(lightSubPathVertices != nullptr);
	assert(numVertices > 0);
	assert(radiusCalculation == CONSTANT_RADIUS || knn > 0);

	// Count number of vertices in medium.
	int numVerticesInMedium = 0;
	for (int i = 0; i < numVertices; i++) {
		if (lightSubPathVertices[i].mInMedium)
			numVerticesInMedium++;
	}

	// Nothing to do.
	if (numVerticesInMedium <= 0)
		return numVerticesInMedium;

	// Allocate embree photons.
	embreePhotons = new EmbreePhoton[numVerticesInMedium];
	numEmbreePhotons = numVerticesInMedium;

	assert(embreePhotons != nullptr);

	KdTree * tree = nullptr;
	KdTree::CKNNQuery * query = nullptr;
	if (radiusCalculation == KNN_RADIUS) {
		tree = new KdTree();
		tree->Reserve(numVerticesInMedium);
		for (int i = 0; i < numVertices; i++) {
			if (lightSubPathVertices[i].mInMedium) {
				tree->AddItem(&lightSubPathVertices[i].mHitpoint, i);
			}
		}
		tree->BuildUp();
		query = new KdTree::CKNNQuery(knn);
	}

	// Convert path vertices to embree photons.
	int inMediumIdx = 0;
	for (int i = 0; i < numVertices; i++) {
		if (lightSubPathVertices[i].mInMedium) {
			const UPBPLightVertex& v = lightSubPathVertices[i];
			float radius = photonRadius;
			if (radiusCalculation == KNN_RADIUS) {
				query->Init(v.mHitpoint, knn, MAX_FLOAT_SQUARE_ROOT);
				// Execute query.
				tree->KNNQuery(*query, tree->truePred);
				assert(query->found > 1);
				radius *= 2.0f * sqrtf(query->dist2[1]);
			}
			embreePhotons[inMediumIdx].set(radius, &v);
			inMediumIdx++;
		}
	}

	if (radiusCalculation == KNN_RADIUS) {
		delete query;
		delete tree;
	}

	assert(inMediumIdx == numVerticesInMedium);

	// Build embree data structure.
	m_rtcScene = BuildPhotonTree(device, embreePhotons, numVerticesInMedium);
	assert(m_rtcScene);

	return numVerticesInMedium;
}

// ------------------------------------------------------------------------ //

/**
 * @brief	Destroy the data structure for BRE queries.
 */
void EmbreeBre::Destroy()
{
	if (embreePhotons) {
		delete[] embreePhotons;
		embreePhotons = nullptr;
	}

	if (m_rtcScene) {
		rtcReleaseScene(m_rtcScene);
		m_rtcScene = nullptr;
	}

	numEmbreePhotons = 0;
}

// ------------------------------------------------------------------------ //

/**
 * @brief	Destructor.
 */
EmbreeBre::~EmbreeBre()
{
	// The user should call destroy manually before deleting this object.
	assert(embreePhotons == nullptr);
	assert(numEmbreePhotons == 0);
	assert(m_rtcScene == nullptr);

	// Destroy anyway even if the user has forgotten to destroy().
	Destroy();
}

// ------------------------------------------------------------------------ //

/**
 * @brief	Evaluates the beam radiance estimate for the given query ray.
 *
 * @param	beamType					   	Type of the beam.
 * @param	queryRay					   	The query ray (=beam) for the beam radiance estimate.
 * @param	segments					   	Full volume segments of media intersected by the ray.
 * @param	estimatorTechniques			   	the estimator techniques to use.
 * @param	raySamplingFlags			   	the ray sampling flags (\c
 * 											kOriginInMedium).
 * @param [in,out]	additionalRayDataForMis	(Optional) additional data needed for MIS weights
 * 											computations.
 *
 * @return	The accumulated radiance along the ray.
 */
vec3 EmbreeBre::EvalBre(BeamType beamType,
						const Ray& queryRay,
						const VolumeSegments& segments,
						const uint estimatorTechniques,
						const uint raySamplingFlags,
						AdditionalRayDataForMis* additionalRayDataForMis) {
	assert(estimatorTechniques & PB2D);
	assert(raySamplingFlags == 0 || raySamplingFlags == kOriginInMedium);

	vec3 result(0.f);

	if (!m_rtcScene)
		return result;

	vec3 attenuation(1.f);
	float raySamplePdf = 1.0f;
	float raySampleRevPdf = 1.0f;

	/// Accumulate for each segment.
	for (auto it = segments.begin(); it != segments.end(); ++it) {
		// Get segment medium.
		auto medium = it->mMedium;

		// Accumulate.
		vec3 segmentResult(0.f);
		if (medium->hasScattering()) {
			if (additionalRayDataForMis) {
				additionalRayDataForMis->mRaySamplePdf = raySamplePdf;
				additionalRayDataForMis->mRaySampleRevPdf = raySampleRevPdf;
				additionalRayDataForMis->mRaySamplingFlags = kEndInMedium;
				if (it == segments.begin())
					additionalRayDataForMis->mRaySamplingFlags |= raySamplingFlags;
			}
			EmbreeRay embreeRay(queryRay.origin(), queryRay.direction(), it->mDistMin, it->mDistMax);
			embreeRay.SetAdditionalData(medium, &segmentResult, beamType | estimatorTechniques, &queryRay, additionalRayDataForMis);
//			rtcIntersect(m_rtcScene, embreeRay);
		}

		// Add to total result.
		result += attenuation * segmentResult;

		// Update attenuation.
		attenuation *= beamType == SHORT_BEAM ? it->mAttenuation / it->mRaySamplePdf :  // Short beams - no attenuation
			it->mAttenuation;
		if (!isPositive(attenuation))
			return result;

		// Update PDFs.
		raySamplePdf *= it->mRaySamplePdf;
		raySampleRevPdf *= it->mRaySampleRevPdf;
	}

	assert(!isNanInfNeg(result));

	return result;
}
