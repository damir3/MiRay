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
 Based on https://github.com/PetrVevoda/smallupbp/blob/master/SmallUPBP/src/Renderers/UPBP.hxx
 Modified by Damir Sagidullin
 */

#include "../../Samplers/Halton.h"
#include "UPBP.h"

#define mMinPathLength		0

#define MULTI_THREAD_LIGHT_PATHS_TRACING

UPBP::UPBP(Scene &					aScene,
		   const AlgorithmType		aAlgorithm,
		   const uint				aEstimatorTechniques,
		   const RadiusCalculation	aPB2DRadiusCalculation,
		   const int				aPB2DRadiusKNN,
		   const BeamType			aQueryBeamType,
		   const RadiusCalculation	aBB1DRadiusCalculation,
		   const int				aBB1DRadiusKNN,
		   const BeamType			aPhotonBeamType,
		   const float				aBB1DBeamStorageFactor,
		   const float				aRefPathCountPerIter,
		   const float				aPathCountPerIter,
		   const float				aMinDistToMed,
		   const size_t				aMaxMemoryPerThread,
		   const int				aBaseSeed)
	: BaseRenderer(aScene)
	, mAlgorithm(aAlgorithm)
	, mPB2DEmbreeBre(aScene)
	, mBB1DPhotonBeams(/*aScene*/)
	, mEstimatorTechniques(aEstimatorTechniques)
	, mSurfRadiusInitial(0.f)
	, mSurfRadiusAlpha(0.85f)
	, mPP3DRadiusInitial(0.f)
	, mPP3DRadiusAlpha(1.f)
	, mPB2DRadiusInitial(0.f)
	, mPB2DRadiusAlpha(1.f)
	, mPB2DRadiusCalculation(aPB2DRadiusCalculation)
	, mPB2DRadiusKNN(aPB2DRadiusKNN)
	, mQueryBeamType(aQueryBeamType)
	, mBB1DRadiusInitial(0.f)
	, mBB1DRadiusAlpha(1.f)
	, mBB1DRadiusCalculation(aBB1DRadiusCalculation)
	, mBB1DRadiusKNN(aBB1DRadiusKNN)
	, mPhotonBeamType(aPhotonBeamType)
	, mBB1DBeamStorageFactor(aBB1DBeamStorageFactor)
	, mRefPathCountPerIter(aRefPathCountPerIter)
	, mPathCountPerIter(aPathCountPerIter)
	, mMinDistToMed(aMinDistToMed)
	, mMaxMemoryPerThread(aMaxMemoryPerThread)
	, mBaseSeed(aBaseSeed)
{
//	if (mAlgorithm == kPPM)
//	{
//		// We will check the scene to make sure it does not contain mixed
//		// specular and non-specular materials
//		for (int i = 0; i < mScene.GetMaterialCount(); ++i)
//		{
//			const Material &mat = mScene.GetMaterial(i);
//
//			const bool hasNonSpecular =
//			(mat.mDiffuseReflectance.max() > 0.f) ||
//			(mat.mPhongReflectance.max() > 0.f);
//
//			const bool hasSpecular =
//			(mat.mMirrorReflectance.max() > 0.f) ||
//			(mat.mIOR > 0.f);
//
//			if (hasNonSpecular && hasSpecular)
//			{
//				printf("*WARNING* Our PPM implementation cannot handle materials mixing\n"
//					   "Specular and NonSpecular BSDFs. The extension would be\n"
//					   "fairly straightforward. In SampleScattering for camera sub-paths\n"
//					   "limit the considered events to Specular only.\n"
//					   "Merging will use non-specular components, scattering will be specular.\n"
//					   "If there is no specular component, the ray will terminate.\n\n");
//
//				printf("We are now switching from *PPM* to *BPM*, which can handle the scene\n\n");
//
//				mAlgorithm = kBPM;
//				break;
//			}
//		}
//	}

	switch (mAlgorithm) {
		case kLT:
			mTraceLightPaths            = true;
			mTraceCameraPaths           = false;
			mConnectToCamera            = true;
			mConnectToLightSource       = false;
			mConnectToLightVertices     = false;
			mMergeWithLightVerticesSurf = false;
			mMergeWithLightVerticesPP3D = false;
			mMergeWithLightVerticesPB2D = false;
			mMergeWithLightVerticesBB1D = false;
			break;
		case kPTdir:
			mTraceLightPaths            = false;
			mTraceCameraPaths           = true;
			mConnectToCamera            = false;
			mConnectToLightSource       = false;
			mConnectToLightVertices     = false;
			mMergeWithLightVerticesSurf = false;
			mMergeWithLightVerticesPP3D = false;
			mMergeWithLightVerticesPB2D = false;
			mMergeWithLightVerticesBB1D = false;
			break;
		case kPTls:
			mTraceLightPaths            = false;
			mTraceCameraPaths           = true;
			mConnectToCamera            = false;
			mConnectToLightSource       = true;
			mConnectToLightVertices     = false;
			mMergeWithLightVerticesSurf = false;
			mMergeWithLightVerticesPP3D = false;
			mMergeWithLightVerticesPB2D = false;
			mMergeWithLightVerticesBB1D = false;
			break;
		case kPTmis:
			mTraceLightPaths            = false;
			mTraceCameraPaths           = true;
			mConnectToCamera            = false;
			mConnectToLightSource       = true;
			mConnectToLightVertices     = false;
			mMergeWithLightVerticesSurf = false;
			mMergeWithLightVerticesPP3D = false;
			mMergeWithLightVerticesPB2D = false;
			mMergeWithLightVerticesBB1D = false;
			break;
		case kBPT:
			mTraceLightPaths            = true;
			mTraceCameraPaths           = true;
			mConnectToCamera            = true;
			mConnectToLightSource       = true;
			mConnectToLightVertices     = true;
			mMergeWithLightVerticesSurf = false;
			mMergeWithLightVerticesPP3D = false;
			mMergeWithLightVerticesPB2D = false;
			mMergeWithLightVerticesBB1D = false;
			break;
		case kPPM:
			mTraceLightPaths            = true;
			mTraceCameraPaths           = true;
			mConnectToCamera            = false;
			mConnectToLightSource       = false;
			mConnectToLightVertices     = false;
			mMergeWithLightVerticesSurf = true;
			mMergeWithLightVerticesPP3D = false;
			mMergeWithLightVerticesPB2D = false;
			mMergeWithLightVerticesBB1D = false;
			break;
		case kBPM:
			mTraceLightPaths            = true;
			mTraceCameraPaths           = true;
			mConnectToCamera            = false;
			mConnectToLightSource       = false;
			mConnectToLightVertices     = false;
			mMergeWithLightVerticesSurf = true;
			mMergeWithLightVerticesPP3D = false;
			mMergeWithLightVerticesPB2D = false;
			mMergeWithLightVerticesBB1D = false;
			break;
		case kVCM:
			mTraceLightPaths            = true;
			mTraceCameraPaths           = true;
			mConnectToCamera            = true;
			mConnectToLightSource       = true;
			mConnectToLightVertices     = true;
			mMergeWithLightVerticesSurf = true;
			mMergeWithLightVerticesPP3D = false;
			mMergeWithLightVerticesPB2D = false;
			mMergeWithLightVerticesBB1D = false;
			break;
		case kCustom:
			mTraceLightPaths            = mEstimatorTechniques != 0;
			mTraceCameraPaths           = mEstimatorTechniques != 0;
			mConnectToCamera            = mEstimatorTechniques & BPT;
			mConnectToLightSource       = mEstimatorTechniques & BPT;
			mConnectToLightVertices     = mEstimatorTechniques & BPT;
			mMergeWithLightVerticesSurf = mEstimatorTechniques & SURF;
			mMergeWithLightVerticesPP3D = mEstimatorTechniques & PP3D;
			mMergeWithLightVerticesPB2D = mEstimatorTechniques & PB2D;
			mMergeWithLightVerticesBB1D = mEstimatorTechniques & BB1D;
			break;
	}

	mConnectToCameraFromSurf = (mEstimatorTechniques & (PREVIOUS|COMPATIBLE)) == 0;

	if (mAlgorithm != kCustom) {
		if (mConnectToLightVertices)		mEstimatorTechniques |= BPT;
		if (mMergeWithLightVerticesSurf)	mEstimatorTechniques |= SURF;
		if (mMergeWithLightVerticesPP3D)	mEstimatorTechniques |= PP3D;
		if (mMergeWithLightVerticesPB2D)	mEstimatorTechniques |= PB2D;
		if (mMergeWithLightVerticesBB1D)	mEstimatorTechniques |= BB1D;
	}
}

// ------------------------------------------------------------------------ //

void UPBP::beginIteration()
{
	const int aIteration = m_iterationIndex;

	m_viewportWidth = m_renderArea.frameSize.x;
	m_viewportHeight = m_renderArea.frameSize.y;
	m_viewportToScreen[0] = vec2(1.f) / m_dp;
	m_viewportToScreen[1] = (vec2(0.5f) - m_po) * m_viewportToScreen[0] + 0.5f;
	m_viewportToScreen[0] *= vec2(0.5f, -0.5f);

	// Get path count, one path for each pixel
	const int resX = m_viewportWidth;
	const int resY = m_viewportHeight;
	int pathCountC = resX * resY;
	int pathCountL = mPathCountPerIter;

	// We don't have the same number of pixels (camera paths)
	// and light paths
	mScreenPixelCount = float(pathCountC);
	mLightSubPathCount = mPathCountPerIter;

	auto & camera = m_scene.camera();
	const auto tanHalfAngle = std::tan(glm::radians(camera.fov().get()) * 0.5f);
	const auto cameraDistance = std::max(camera.depthOfField().get() ? camera.focusDistance().get() : camera.distance().get(), camera.farZ().get() * 0.02f);
	mImagePlaneDist = (float)resY / (2.f * tanHalfAngle);
	const auto cameraAspect = camera.aspect().get();
	const auto screenAspect = (float)resX / (float)resY;
	if (screenAspect < cameraAspect)
		mImagePlaneDist *= screenAspect / cameraAspect;

	// To make list of photons and beams same in previous and compatible mode
	mBB1DPhotonBeams.mSeed = mBaseSeed + aIteration;

	// Setup our radius, 1st iteration has aIteration == 0, thus offset
	auto baseRadius = m_photonScale * cameraDistance * tanHalfAngle / (float)m_frameBuffer->height();

	mSurfRadiusInitial = 4.f * baseRadius;
	assert(mSurfRadiusInitial > 0.f);

	mPP3DRadiusInitial = 0.5f * baseRadius;
	assert(mPP3DRadiusInitial > 0.f);

	mPB2DRadiusInitial = 0.5f * baseRadius;
	assert(mPB2DRadiusInitial > 0.f);

	mBB1DRadiusInitial = 0.5f * baseRadius;
	assert(mBB1DRadiusInitial > 0.f);

	mMinDistToMed = 0.f;// 0 * m_sceneSphere.radius;
	assert(mMinDistToMed >= 0);

	mBB1DUsedLightSubPathCount = 0x4000;

	// Radius reduction (1st iteration has aIteration == 0, thus offset)
	const float effectiveIteration = 1 + aIteration * mLightSubPathCount / mRefPathCountPerIter;
	// SURF
	m_radiusSurf = mSurfRadiusInitial * std::pow(effectiveIteration, (mSurfRadiusAlpha - 1) * 0.5f);
	m_radiusSurf = std::max(m_radiusSurf, 1e-7f); // Purely for numeric stability
	const float radiusSurfSqr = sqr(m_radiusSurf);
	// PP3D
	m_radiusPP3D = mPP3DRadiusInitial * std::pow(effectiveIteration, (mPP3DRadiusAlpha - 1) * (1.f / 3.f));
	m_radiusPP3D = std::max(m_radiusPP3D, 1e-7f); // Purely for numeric stability
	const float radiusPP3DCube = sqr(m_radiusPP3D) * m_radiusPP3D;
	// PB2D
	m_radiusPB2D = mPB2DRadiusInitial * std::pow(effectiveIteration, (mPB2DRadiusAlpha - 1) * 0.5f);
	m_radiusPB2D = std::max(m_radiusPB2D, 1e-7f); // Purely for numeric stability
	const float radiusPB2DSqr = sqr(m_radiusPB2D);
	// BB1D
	m_radiusBB1D = mBB1DRadiusInitial * std::pow(1 + aIteration * mBB1DUsedLightSubPathCount / mRefPathCountPerIter, mBB1DRadiusAlpha - 1);
	m_radiusBB1D = std::max(m_radiusBB1D, 1e-7f); // Purely for numeric stability

//	printf("%d: %f %f %f %f\n", aIteration, m_radiusSurf / mSurfRadiusInitial,
//		   m_radiusPP3D / mPP3DRadiusInitial, m_radiusPB2D / mPB2DRadiusInitial, m_radiusBB1D / mBB1DRadiusInitial);

	// Constant for decision whether to store beams or not
	mBB1DMinMFP = mBB1DBeamStorageFactor * 0.5f * M_PIf * m_radiusBB1D;

	const float etaSurf = (M_PIf * radiusSurfSqr) * mLightSubPathCount;
	const float etaPP3D = (4.0f / 3.0f) * (M_PIf * radiusPP3DCube) * mLightSubPathCount;
	const float etaPB2D = (M_PIf * radiusPB2DSqr) * mLightSubPathCount;
	const float etaBB1D = 0.5f * m_radiusBB1D * mBB1DUsedLightSubPathCount;

	// Factor used to normalize vertex merging contribution.
	// We divide the summed up energy by disk radius and number of light paths
	mSurfNormalization = 1.f / etaSurf;
	mPP3DNormalization = 1.f / etaPP3D;
	mPB2DNormalization = 1.f / mLightSubPathCount;
	mBB1DNormalization = 1.f / mBB1DUsedLightSubPathCount;

	// MIS weight constants
	mSurfMisWeightFactor = etaSurf;
	mPP3DMisWeightFactor = etaPP3D;
	mPB2DMisWeightFactor = etaPB2D;
	mBB1DMisWeightFactor = etaBB1D;

	// Clear path ends, nothing ends anywhere
	mPathSegments.resize(pathCountL);
	memset(mPathSegments.data(), 0, mPathSegments.size() * sizeof(PathSegment));

	// Because of static mCameraVerticesMisData size
	assert(m_maxPathLength < UPBP_CAMERA_MAXVERTS);

	const size_t maxLightVerts = std::min<size_t>(mLightSubPathCount * std::min((int)m_maxPathLength, UPBP_LIGHT_AVGVERTS), mMaxMemoryPerThread / sizeof(UPBPLightVertex));
	const size_t maxBeams = std::min<size_t>(mBB1DUsedLightSubPathCount * std::min((int)m_maxPathLength, UPBP_LIGHT_AVGVERTS), mMaxMemoryPerThread / sizeof(UPBPLightVertex));

	// Remove all photon beams and reserve space for some
	mPhotonBeamsArray.clear();
	mPhotonBeamsArray.reserve(maxBeams);
	assert(mPhotonBeamsArray.size() == 0 && mPhotonBeamsArray.capacity() >= maxBeams);

	mLightVertexCount = 0;
	mLightVerticesOnSurfaceCount = 0;
	mLightVerticesInMediumCount = 0;

	if (mTraceLightPaths && m_lightCount > 0 && m_maxPathLength > 1) {
//		auto time1 = std::chrono::high_resolution_clock::now();

#ifndef MULTI_THREAD_LIGHT_PATHS_TRACING
		// Remove all light vertices and reserve space for some
		mLightVertices.clear();
		mLightVertices.reserve(maxLightVerts);
		mThreadCameraPhotons.resize(1);
		mThreadCameraPhotons.front().clear();

		assert(mLightVertices.size() == 0 && mLightVertices.capacity() >= maxLightVerts);

		int resolution = (int)std::ceil(std::sqrt(mPathCountPerIter));
		int resLog2 = log2i(resolution);
		resolution = 1 << resLog2;

		HaltonSampler sampler;
		sampler.setResolution(resolution, resolution);

		for (int pathIdx = 0; pathIdx < pathCountL && !m_interrupted; pathIdx++) {
			mPathSegments[pathIdx].begin = (int)mLightVertices.size();
			sampler.Init(pathIdx & (resolution - 1), pathIdx >> resLog2, m_frameNumber);
			traceLightPath(pathIdx, mLightVertices, mThreadCameraPhotons.front(), mPhotonBeamsArray, sampler);
			mPathSegments[pathIdx].end = (int)mLightVertices.size();
		}

		mLightVertexCount = mLightVertices.size();
		mPhotonBeamsCount = mPhotonBeamsArray.size();
#else
		mLightVertices.resize(maxLightVerts);
		m_lightPathIndex = 0;
		m_lightVertexIndex = 0;

		const auto numThreads = std::max<int>(m_numCPU - 1, 1);
		mThreadCameraPhotons.resize(numThreads);
		mThreadPhotonBeamsArray.resize(numThreads);
		std::vector< std::future<void> > ftasks;
		for (int i = 0; i < numThreads; i++)
			ftasks.emplace_back(std::async(std::launch::async, &UPBP::traceLightPathsThreadFunc, this, i));

		for (auto & f: ftasks)
			f.wait();

		mLightVertexCount = std::min<size_t>(m_lightVertexIndex, maxLightVerts);
		mPhotonBeamsCount = 0;
		for (auto & array : mThreadPhotonBeamsArray)
			mPhotonBeamsCount += array.size();
#endif

		if (m_interrupted)
			return;

//		auto time2 = std::chrono::high_resolution_clock::now();

		buildAccelerationStructures();

//		auto time3 = std::chrono::high_resolution_clock::now();

//		size_t cameraPhotonsCount = 0;
//		for (const auto & photons : mThreadCameraPhotons)
//			cameraPhotonsCount += photons.size();

//		printf("(%d / %.1f) (%d / %.1f) %d ", (int)mLightVertexCount, std::chrono::duration<double>(time2 - time1).count() * 1e3,
//			   (int)mPhotonBeamsCount, std::chrono::duration<double>(time3 - time2).count() * 1e3, (int)cameraPhotonsCount);
	}
}

// ------------------------------------------------------------------------ //

void UPBP::traceLightPathsThreadFunc(int threadIndex)
{
	//	auto time1 = std::chrono::high_resolution_clock::now();
	QThread::currentThread()->setPriority(QThread::LowestPriority);

	std::vector<UPBPLightVertex> lightVertices;
	lightVertices.reserve(m_maxPathLength);

	auto & cameraPhotons = mThreadCameraPhotons[threadIndex];
	cameraPhotons.clear();

	auto & photonBeamsArray = mThreadPhotonBeamsArray[threadIndex];
	photonBeamsArray.reserve(mPhotonBeamsArray.capacity());
	photonBeamsArray.clear();

	int resolution = (int)std::ceil(std::sqrt(mPathCountPerIter));
	int resLog2 = log2i(resolution);
	resolution = 1 << resLog2;

	HaltonSampler sampler;
	sampler.setResolution(resolution, resolution);

	while (!m_interrupted) {
		enum { BLOCK_SIZE = 256 };
		const auto beginIndex = m_lightPathIndex.fetch_add(BLOCK_SIZE);
		if (beginIndex >= (size_t)mPathCountPerIter) break;
		const auto endIndex = std::min(beginIndex + BLOCK_SIZE, (size_t)mPathCountPerIter);

		for (auto pathIndex = beginIndex; pathIndex < endIndex; pathIndex++) {
			assert(pathIndex < mPathSegments.size());
			lightVertices.clear();
			size_t pathBeamIndex = photonBeamsArray.size();
			sampler.init((int)pathIndex & (resolution - 1), (int)pathIndex >> resLog2, m_iterationIndex);
			traceLightPath((int)pathIndex, lightVertices, cameraPhotons, photonBeamsArray, sampler);

			int numVertices = (int)lightVertices.size();
			int pathVertex = (int)m_lightVertexIndex.fetch_add(numVertices);
			assert(pathVertex + numVertices <= (int)mLightVertices.size());
			if (pathVertex + numVertices > (int)mLightVertices.size())
				numVertices = std::max<int>((int)mLightVertices.size() - pathVertex, 0);
			mPathSegments[pathIndex] = PathSegment{ pathVertex, pathVertex + numVertices };
			memcpy(mLightVertices.data() + pathVertex, lightVertices.data(), numVertices * sizeof(UPBPLightVertex));
			while (pathBeamIndex < photonBeamsArray.size()) {
				auto & beam = photonBeamsArray[pathBeamIndex++];
				beam.mLightVertex = mLightVertices.data() + pathVertex + (beam.mLightVertex - lightVertices.data());
			}
		}
	}

	//	auto time2 = std::chrono::high_resolution_clock::now();
	//	printf("(%d: %.1f\n", threadIndex, std::chrono::duration<double>(time2 - time1).count() * 1e3);
}

// ------------------------------------------------------------------------ //

void UPBP::traceLightPath(int pathIdx, std::vector<UPBPLightVertex> & oLightVertices, CameraPhotonsArray & oCameraPhotons, PhotonBeamsArray & oPhotonBeamsArray, Sampler & sampler)
{
	const UPBPLightVertex * pathVertices = oLightVertices.data() + oLightVertices.size();

	// Generate light path origin and direction
	SubPathState lightState;
	if (!generateLightSample(pathIdx, lightState, oLightVertices, sampler))
		return;

	// In attenuating media the ray can never travel from infinity
//	if (!lightState.mIsFiniteLight && mScene.GetGlobalMediumPtr()->HasAttenuation())
//		return;

	// We assume that the light is on surface
	bool originInMedium = false;

	VolumeSegments	mVolumeSegments;		// Path segments intersecting media (up to scattering point)
	TraceContext ctx(vec3(0.f), vec3(0.f), 0.f, sampler, pathIdx);
	Ray & ray = ctx.ray;
	ray.boundaryStack = &lightState.mBoundaryStack;

	//////////////////////////////////////////////////////////////////////////
	// Trace light path
	for (;; ++lightState.mPathLength) {
		// Prepare ray
		ray.setOrigin(lightState.mOrigin);
		ray.setDirection(lightState.mDirection);
		ray.ray.tnear = 0.f;
		ray.ray.tfar = m_scene.sceneSphere().radius * 2.f;
		Isect isect(1e36f);

		// Trace ray
		mVolumeSegments.clear();
//		mLiteVolumeSegments.clear();
		bool intersected = intersect(ray, isect, sampler, lightState.mBoundaryStack, kSampleVolumeScattering, originInMedium ? kOriginInMedium : 0, &mVolumeSegments);

		// Store beam if required
		if (mMergeWithLightVerticesBB1D && pathIdx < mBB1DUsedLightSubPathCount) {
			addBeams(ray, lightState.mThroughput, mVolumeSegments, &oLightVertices.back(), originInMedium ? kOriginInMedium : 0, lightState.mLastPdfWInv, oPhotonBeamsArray);
		}

		if (!intersected)
			break;

		assert(isect.isValid());

		// Attenuate by intersected media (if any)
		float raySamplePdf(1.f);
		float raySampleRevPdf(1.f);
		if (!mVolumeSegments.empty()) {
			// PDF
			raySamplePdf = VolumeSegment::AccumulatePdf(mVolumeSegments);
			assert(raySamplePdf > 0.f);

			// Reverse PDF
			raySampleRevPdf = VolumeSegment::AccumulateRevPdf(mVolumeSegments);
			assert(raySampleRevPdf > 0.f);

			// Attenuation
			lightState.mThroughput *= VolumeSegment::AccumulateAttenuationWithoutPdf(mVolumeSegments) / raySamplePdf;
		}

		if (isBlack(lightState.mThroughput))
			break;

		// Prepare scattering function at the hitpoint (BSDF/phase depending on whether the hitpoint is at surface or in media, the isect knows)
		BSDF bsdf;
		if (isect.mFloor) {
			if (!bsdf.setupFloor(ctx, isect, 1.f, m_floor, true))
				break;
		} else {
			if (!bsdf.setup(ctx, isect, lightState.mBoundaryStack, true))
				break;
		}

		if (bsdf.continuationProbability() == 0.f)
			break;

		// Compute hitpoint
		const vec3 hitPoint = ray.hitPoint;

		originInMedium = isect.isInMedium();

		// Store vertex
		{
			UPBPLightVertex lightVertex;
			lightVertex.mHitpoint = hitPoint;
			lightVertex.mThroughput = lightState.mThroughput;
			lightVertex.mPathIdx = pathIdx;
			lightVertex.mPathLength = lightState.mPathLength;
			lightVertex.mInMedium = originInMedium;
			lightVertex.mConnectable = !bsdf.isDelta();
			lightVertex.mIsFinite = true;
			lightVertex.mBSDF = bsdf;

			// Determine whether the vertex is in medium behind real geometry
			lightVertex.mBehindSurf = false;
			if (lightVertex.mInMedium && !lightState.mBoundaryStack.IsEmpty()) {
				auto mat = lightState.mBoundaryStack.Top().mMedium;
				if (mat) {
//					if (mat->mGeometryType != GeometryType::IMAGINARY)
						lightVertex.mBehindSurf = true;
				}
			}

			// Infinite lights use MIS handled via solid angle integration, so do not divide by the distance for such lights
			const float distSq = (lightState.mPathLength > 1 || lightState.mIsFiniteLight == 1) ? sqr(isect.mDist) : 1.f;
			const float raySamplePdfInv = 1.f / raySamplePdf;
			lightVertex.mMisData.mPdfAInv = lightState.mLastPdfWInv * distSq * raySamplePdfInv / std::fabs(bsdf.cosThetaFix());
			lightVertex.mMisData.mRevPdfA = 1.f;
			lightVertex.mMisData.mRevPdfAWithoutBsdf = lightVertex.mMisData.mRevPdfA;
			lightVertex.mMisData.mRaySamplePdfInv = raySamplePdfInv;
			lightVertex.mMisData.mRaySampleRevPdfInv = 1.f;
			lightVertex.mMisData.mSinTheta = 0.f;
			lightVertex.mMisData.mCosThetaOut = 0.f;
			lightVertex.mMisData.mSurfMisWeightFactor = bsdf.isOnSurface() ? mSurfMisWeightFactor : 0;
			lightVertex.mMisData.mPP3DMisWeightFactor = bsdf.isOnSurface() ? 0 : mPP3DMisWeightFactor;
			lightVertex.mMisData.mPB2DMisWeightFactor = bsdf.isOnSurface() ? 0 : mPB2DMisWeightFactor;
			lightVertex.mMisData.mBB1DMisWeightFactor = bsdf.isOnSurface() ? 0 : mBB1DMisWeightFactor;
			lightVertex.mMisData.mBB1DBeamSelectionPdf = bsdf.isOnSurface() ? 0 : 1;
			lightVertex.mMisData.mIsDelta = bsdf.isDelta();
			lightVertex.mMisData.mIsOnLightSource = false;
			lightVertex.mMisData.mIsSpecular = false;
			lightVertex.mMisData.mInMediumWithBeams = bsdf.isOnSurface() ? false : (!mMergeWithLightVerticesPB2D || bsdf.medium()->getMeanFreePath(hitPoint) > mBB1DMinMFP);

			lightVertex.mMisData.mRaySamplePdfsRatio = 0.f;
			lightVertex.mMisData.mRaySampleRevPdfsRatio = 0.f;
			if (bsdf.isInMedium()) {
				if (bsdf.medium()->isHomogeneous()) {
					lightVertex.mMisData.mRaySamplePdfsRatio = 1.f / ((const MaterialImpl*)bsdf.medium())->mMinPositiveAttenuationCoefComp();
					lightVertex.mMisData.mRaySampleRevPdfsRatio = lightVertex.mMisData.mRaySamplePdfsRatio;
				} else {
					const float lastSegmentRayOverSamplePdf = bsdf.medium()->raySamplePdf(mVolumeSegments.back().mDistMin, mVolumeSegments.back().mDistMax, 0);
					const float lastSegmentRayInSamplePdf = mVolumeSegments.back().mRaySamplePdf; // We are in medium -> we know we have insampled
					lightVertex.mMisData.mRaySamplePdfsRatio = lastSegmentRayOverSamplePdf / lastSegmentRayInSamplePdf;
				}
			}

			// Update reverse PDFs of the previous vertex
			oLightVertices.back().mMisData.mRevPdfA *= raySampleRevPdf / distSq;
			oLightVertices.back().mMisData.mRevPdfAWithoutBsdf = oLightVertices.back().mMisData.mRevPdfA;
			oLightVertices.back().mMisData.mRaySampleRevPdfInv = 1.f / raySampleRevPdf;

			if (oLightVertices.back().mBSDF.isInMedium() && !oLightVertices.back().mBSDF.medium()->isHomogeneous()) { // Homogeneous case was solved immediately when processing the vertex for the first time
				float firstSegmentRayOverSampleRevPdf;
				oLightVertices.back().mBSDF.medium()->raySamplePdf(mVolumeSegments.front().mDistMin, mVolumeSegments.front().mDistMax, 0, &firstSegmentRayOverSampleRevPdf);
				const float firstSegmentRayInSampleRevPdf = mVolumeSegments.front().mRaySampleRevPdf; // We were in medium -> we know we have insampled
				oLightVertices.back().mMisData.mRaySampleRevPdfsRatio = firstSegmentRayOverSampleRevPdf / firstSegmentRayInSampleRevPdf;
			}

			if (lightVertex.mInMedium)
				mLightVerticesInMediumCount++;
			else
				mLightVerticesOnSurfaceCount++;

			assert(oLightVertices.size() < oLightVertices.capacity());
			oLightVertices.emplace_back(lightVertex);
		}

		// Connect to camera, unless scattering function is purely specular or we are not allowed to connect from surface
		if (mConnectToCamera && !bsdf.isDelta() && (bsdf.isInMedium() || mConnectToCameraFromSurf)) {
//			if (lightState.mPathLength + 1 >= mMinPathLength)
				connectToCamera(pathVertices, lightState, hitPoint, bsdf, oLightVertices.back().mMisData.mRaySamplePdfsRatio, ctx, oCameraPhotons);
		}

		if (isect.mLight)
			break;

		// Terminate if the path would become too long after scattering
		if (lightState.mPathLength + 2 > m_maxPathLength)
			break;

		// Continue random walk
		if (!sampleScattering(bsdf, hitPoint, isect, lightState,
							  oLightVertices.back().mMisData, oLightVertices.at(oLightVertices.size() - 2).mMisData, ctx))
			break;
	}
}

void UPBP::buildSurfHashGrid()
{
//	auto time1 = std::chrono::high_resolution_clock::now();

	// The number of cells is somewhat arbitrary, but seems to work ok
	mSurfHashGrid.Reserve(mPathCountPerIter);
	mSurfHashGrid.Build(mLightVertices.data(), mLightVertexCount, m_radiusSurf, SURF);

//	auto time2 = std::chrono::high_resolution_clock::now();
//	printf("BuildSurfHashGrid %.1f\n", std::chrono::duration<double>(time2 - time1).count() * 1e3);
}

void UPBP::buildPP3DHashGrid()
{
//	auto time1 = std::chrono::high_resolution_clock::now();

	// The number of cells is somewhat arbitrary, but seems to work ok
	mPP3DHashGrid.Reserve(mPathCountPerIter);
	mPP3DHashGrid.Build(mLightVertices.data(), mLightVertexCount, m_radiusPP3D, PP3D);

//	auto time2 = std::chrono::high_resolution_clock::now();
//	printf("BuildPP3DHashGrid %.1f\n", std::chrono::duration<double>(time2 - time1).count() * 1e3);
}

void UPBP::buildPB2DEmbreeBre()
{
//	auto time1 = std::chrono::high_resolution_clock::now();

	mPB2DEmbreeBre.Build(m_scene.rtcDevice(), mLightVertices.data(), (int)mLightVertexCount, mPB2DRadiusCalculation, m_radiusPB2D, mPB2DRadiusKNN);

//	auto time2 = std::chrono::high_resolution_clock::now();
//	printf("BuildPB2DEmbreeBre %.1f\n", std::chrono::duration<double>(time2 - time1).count() * 1e3);
}

void UPBP::buildBB1DPhotonBeams()
{
//	auto time1 = std::chrono::high_resolution_clock::now();

#ifdef MULTI_THREAD_LIGHT_PATHS_TRACING
	mPhotonBeamsArray.resize(mPhotonBeamsCount);
	size_t count = 0;
	for (auto & array : mThreadPhotonBeamsArray) {
		memcpy(mPhotonBeamsArray.data() + count, array.data(), array.size() * sizeof(PhotonBeam));
		count += array.size();
		array.clear();
	}
	assert(count == mPhotonBeamsCount);
#endif

	mBB1DPhotonBeams.build(mPhotonBeamsArray, mBB1DRadiusCalculation, m_radiusBB1D, mBB1DRadiusKNN);

	// Set beam selection PDFs according to the built structure
	if (mBB1DPhotonBeams.sMaxBeamsInCell) {
		for (auto i = mLightVertices.begin(), iend = mLightVertices.begin() + mLightVertexCount; i != iend; ++i) {
			if (i->mBSDF.isInMedium())
				i->mMisData.mBB1DBeamSelectionPdf = mBB1DPhotonBeams.getBeamSelectionPdf(i->mHitpoint);
		}
	}

//	auto time2 = std::chrono::high_resolution_clock::now();
//	printf("BuildBB1DPhotonBeams %.1f\n", std::chrono::duration<double>(time2 - time1).count() * 1e3);
}

void UPBP::buildAccelerationStructures()
{
	if (m_maxPathLength > 1) {
		std::vector<std::future<void>> ftasks;

		if (mLightVertexCount > 0) {
			//////////////////////////////////////////////////////////////////////////
			// Build acceleration structure for SURF
			//////////////////////////////////////////////////////////////////////////
			if (mMergeWithLightVerticesSurf && mLightVerticesOnSurfaceCount) {
//				BuildSurfHashGrid();
				ftasks.emplace_back(std::async(std::launch::async, &UPBP::buildSurfHashGrid, this));
			}

			//////////////////////////////////////////////////////////////////////////
			// Build acceleration structure for PP3D
			//////////////////////////////////////////////////////////////////////////
			if (mMergeWithLightVerticesPP3D && mLightVerticesInMediumCount) {
//				BuildPP3DHashGrid();
				ftasks.emplace_back(std::async(std::launch::async, &UPBP::buildPP3DHashGrid, this));
			}

			//////////////////////////////////////////////////////////////////////////
			// Build acceleration structure for PB2D
			//////////////////////////////////////////////////////////////////////////
			if (mMergeWithLightVerticesPB2D) {
//				BuildPB2DEmbreeBre();
				ftasks.emplace_back(std::async(std::launch::async, &UPBP::buildPB2DEmbreeBre, this));
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// Build acceleration structure for BB1D
		//////////////////////////////////////////////////////////////////////////
		if (mMergeWithLightVerticesBB1D && mPhotonBeamsCount > 0) {
//			BuildBB1DPhotonBeams();
			ftasks.emplace_back(std::async(std::launch::async, &UPBP::buildBB1DPhotonBeams, this));
		}

		for (auto & f: ftasks)
			f.wait();
	}
}

void UPBP::traceCameraPath(BaseRenderer::TraceResult &res, TraceContext &ctx)
{
	// Generate camera path origin and direction
	SubPathState cameraState;
	generateCameraSample(ctx, cameraState);
	Ray & ray = ctx.ray;
	ray.boundaryStack = &cameraState.mBoundaryStack;

	// We assume that the camera is on surface
	bool originInMedium = false;

	// Medium of the previous vertex
	const MaterialImpl* lastMedium = nullptr;

	bool onlySpecSurf = (mEstimatorTechniques & (PREVIOUS | COMPATIBLE)) != 0;
	bool stopBB1D = false;

	VolumeSegments	mVolumeSegments;		// Path segments intersecting media (up to scattering point)
	MisData mCameraVerticesMisData[UPBP_CAMERA_MAXVERTS]; // Stored MIS data for camera vertices (we don't need store whole vertices as for light paths)
	cameraState.mCameraVerticesMisData = mCameraVerticesMisData;

	Isect isect(1e36f);

	//////////////////////////////////////////////////////////////////////
	// Trace camera path
	for (;; ++cameraState.mPathLength) {
		// Trace ray
		mVolumeSegments.clear();
//		mLiteVolumeSegments.clear();
		if (!intersect(ray, isect, ctx.sampler, cameraState.mBoundaryStack, kSampleVolumeScattering, originInMedium ? kOriginInMedium : 0, &mVolumeSegments)) {
			//assert(!mScene.GetGlobalMediumPtr()->HasScattering());

			// Vertex merging: point x beam 2D
			if (mMergeWithLightVerticesPB2D && mLightVertexCount > 0) {
				uint estimatorTechniques = mEstimatorTechniques;
				//if (!cameraState.mSpecularPath) estimatorTechniques |= BEAM_REDUCTION;
				AdditionalRayDataForMis data(mLightVertices.data(), mPathSegments.data(), mCameraVerticesMisData, cameraState.mPathLength, mMinPathLength, m_maxPathLength, mQueryBeamType, mPhotonBeamType, cameraState.mLastPdfWInv, mSurfMisWeightFactor, mPP3DMisWeightFactor, mPB2DMisWeightFactor, mBB1DMisWeightFactor, (mMergeWithLightVerticesBB1D && mPhotonBeamsCount > 0) ? &mBB1DPhotonBeams : nullptr, mBB1DMinMFP, mLightSubPathCount, mMinDistToMed, 0.f, 0.f, 0);
				const vec3 contrib = mPB2DEmbreeBre.EvalBre(mQueryBeamType, ray, mVolumeSegments, estimatorTechniques, originInMedium ? kOriginInMedium : 0, &data);
				const vec3 mult = cameraState.mThroughput * mPB2DNormalization;
				res.addColor(mult * contrib);
			}

			// Vertex merging: beam x beam 1D
			if (mMergeWithLightVerticesBB1D && mPhotonBeamsCount > 0 && !stopBB1D) {
				uint estimatorTechniques = mEstimatorTechniques;
				//if (!cameraState.mSpecularPath) estimatorTechniques |= BEAM_REDUCTION;
				AdditionalRayDataForMis data(mLightVertices.data(), mPathSegments.data(), mCameraVerticesMisData, cameraState.mPathLength, mMinPathLength, m_maxPathLength, mQueryBeamType, mPhotonBeamType, cameraState.mLastPdfWInv, mSurfMisWeightFactor, mPP3DMisWeightFactor, mPB2DMisWeightFactor, mBB1DMisWeightFactor, mPhotonBeamsCount > 0 ? &mBB1DPhotonBeams : nullptr, mBB1DMinMFP, mBB1DUsedLightSubPathCount, mMinDistToMed, 0.f, 0.f, 0);
				const vec3 contrib = mBB1DPhotonBeams.evalBeamBeamEstimate(mQueryBeamType, ray, mVolumeSegments, estimatorTechniques, originInMedium ? kOriginInMedium : 0, &data);
				const vec3 mult = cameraState.mThroughput * mBB1DNormalization;
				res.addColor(mult * contrib);
			}

			// We cannot end yet
//			if (cameraState.mPathLength < mMinPathLength)
//				break;

			if (isect.mFloor) {
				if (ray.direction().z < 0.f && ray.origin().z > m_floor.level) {
					auto opacity = evaluateShadowOpacity(res, ctx, cameraState.mBoundaryStack, mVolumeSegments);
					if (cameraState.mPathLength == 1) {
						res.opacity = vec3(opacity);
						res.normal = vec3(0.f, 0.f, 1.f);
						res.depth = -ctx.ray.origin().z / ctx.ray.direction().z;
						break;
					}

					cameraState.mThroughput *= 1.f - opacity;
				}
			}

			if (ctx.floor) { // restore ray state for environment lighting
				ray.setOrigin(ctx.floorRayOrigin);
				ray.setDirection(ctx.floorRayDirection);
			}

			// In attenuating media the ray can never travel to infinity
//			if (mScene.GetGlobalMediumPtr()->HasAttenuation())
//				break;

			// Stop if we are in the light sampling mode and could have sampled this light last time in the next event estimation
			if (mAlgorithm == kPTls && cameraState.mPathLength > 1 && !cameraState.mLastSpecular)
				break;

			// Attenuate by intersected media (if any)
			float raySamplePdf(1.f);
			float raySampleRevPdf(1.f);
			if (!mVolumeSegments.empty()) {
				// PDF
				raySamplePdf = VolumeSegment::AccumulatePdf(mVolumeSegments);
				assert(raySamplePdf > 0.f);

				// Reverse PDF
				raySampleRevPdf = VolumeSegment::AccumulateRevPdf(mVolumeSegments);
				assert(raySampleRevPdf > 0.f);

				// Attenuation
				cameraState.mThroughput *= VolumeSegment::AccumulateAttenuationWithoutPdf(mVolumeSegments) / raySamplePdf;
			}

			if (!ctx.reflected && m_bgMode != BackgroundMode_Environment) {
				res.opacity = vec3(1.f) - cameraState.mThroughput;
				break;
			}

			if (isBlack(cameraState.mThroughput))
				break;

			// Update affected MIS data
			const float raySamplePdfInv = 1.f / raySamplePdf;
			mCameraVerticesMisData[cameraState.mPathLength].mPdfAInv = cameraState.mLastPdfWInv * raySamplePdfInv;
			mCameraVerticesMisData[cameraState.mPathLength].mRevPdfA = 1.f;
			mCameraVerticesMisData[cameraState.mPathLength].mRaySamplePdfInv = raySamplePdfInv;
			mCameraVerticesMisData[cameraState.mPathLength].mRaySampleRevPdfInv = 1.f;
			mCameraVerticesMisData[cameraState.mPathLength].mRaySamplePdfsRatio = 0.f;
			mCameraVerticesMisData[cameraState.mPathLength].mSinTheta = 0.f;
			mCameraVerticesMisData[cameraState.mPathLength].mCosThetaOut = 0.f;
			mCameraVerticesMisData[cameraState.mPathLength].mSurfMisWeightFactor = 0.f;
			mCameraVerticesMisData[cameraState.mPathLength].mPP3DMisWeightFactor = 0.f;
			mCameraVerticesMisData[cameraState.mPathLength].mPB2DMisWeightFactor = 0.f;
			mCameraVerticesMisData[cameraState.mPathLength].mBB1DMisWeightFactor = 0.f;
			mCameraVerticesMisData[cameraState.mPathLength].mBB1DBeamSelectionPdf = 0.f;
			mCameraVerticesMisData[cameraState.mPathLength].mIsDelta = false;
			mCameraVerticesMisData[cameraState.mPathLength].mIsOnLightSource = true;
			mCameraVerticesMisData[cameraState.mPathLength].mIsSpecular = false;
			mCameraVerticesMisData[cameraState.mPathLength].mInMediumWithBeams = false;
			mCameraVerticesMisData[cameraState.mPathLength - 1].mRevPdfA *= raySampleRevPdf;
			mCameraVerticesMisData[cameraState.mPathLength - 1].mRaySampleRevPdfInv = 1.f / raySampleRevPdf;

			if (lastMedium && !lastMedium->isHomogeneous()) { // Homogeneous case was solved immediately when processing the vertex for the first time
				float firstSegmentRayOverSampleRevPdf;
				lastMedium->raySamplePdf(mVolumeSegments.front().mDistMin, mVolumeSegments.front().mDistMax, 0, &firstSegmentRayOverSampleRevPdf);
				const float firstSegmentRayInSampleRevPdf = mVolumeSegments.front().mRaySampleRevPdf; // We were in medium -> we know we have insampled
				mCameraVerticesMisData[cameraState.mPathLength - 1].mRaySampleRevPdfsRatio = firstSegmentRayOverSampleRevPdf / firstSegmentRayInSampleRevPdf;
			}

			// Accumulate contribution
			for (const auto * light : m_infiniteLights)
				res.addColor(cameraState.mThroughput * getLightRadiance(light, cameraState, ray));

			break;
		}

		if (cameraState.mPathLength == 1) {
			res.normal = ctx.ray.normal;
			res.depth = ctx.ray.ray.tfar;
			res.node = ctx.ray.node;
			res.geometry = ctx.ray.geom;
		}

		assert(isect.isValid());

		////////////////////////////////////////////////////////////////
		// Vertex merging: point x beam 2D
		if (mMergeWithLightVerticesPB2D && mLightVertexCount > 0) {
			vec3 contrib(0.f);
			uint estimatorTechniques = mEstimatorTechniques;
			//if (!cameraState.mSpecularPath) estimatorTechniques |= BEAM_REDUCTION;
			AdditionalRayDataForMis data(mLightVertices.data(), mPathSegments.data(), mCameraVerticesMisData, cameraState.mPathLength, mMinPathLength, m_maxPathLength, mQueryBeamType, mPhotonBeamType, cameraState.mLastPdfWInv, mSurfMisWeightFactor, mPP3DMisWeightFactor, mPB2DMisWeightFactor, mBB1DMisWeightFactor, (mMergeWithLightVerticesBB1D && mPhotonBeamsCount > 0) ? &mBB1DPhotonBeams : nullptr, mBB1DMinMFP, mLightSubPathCount, mMinDistToMed, 0.f, 0.f, 0);
//			if (isect.isOnSurface() || mQueryBeamType == SHORT_BEAM)
				contrib = mPB2DEmbreeBre.EvalBre(mQueryBeamType, ray, mVolumeSegments, estimatorTechniques, originInMedium ? kOriginInMedium : 0, &data);
//			else
//				contrib = mPB2DEmbreeBre.EvalBre(mQueryBeamType, ray, mLiteVolumeSegments, estimatorTechniques, originInMedium ? kOriginInMedium : 0, &data);
			const vec3 mult = cameraState.mThroughput * mPB2DNormalization;
			res.addColor(mult * contrib);
		}

		////////////////////////////////////////////////////////////////
		// Vertex merging: beam x beam 1D
		if (mMergeWithLightVerticesBB1D && mPhotonBeamsCount > 0 && !stopBB1D) {
			vec3 contrib(0.f);
			uint estimatorTechniques = mEstimatorTechniques;
			//if (!cameraState.mSpecularPath) estimatorTechniques |= BEAM_REDUCTION;
			AdditionalRayDataForMis data(mLightVertices.data(), mPathSegments.data(), mCameraVerticesMisData, cameraState.mPathLength, mMinPathLength, m_maxPathLength, mQueryBeamType, mPhotonBeamType, cameraState.mLastPdfWInv, mSurfMisWeightFactor, mPP3DMisWeightFactor, mPB2DMisWeightFactor, mBB1DMisWeightFactor, mPhotonBeamsCount > 0 ? &mBB1DPhotonBeams : nullptr, mBB1DMinMFP, mBB1DUsedLightSubPathCount, mMinDistToMed, 0.f, 0.f, 0);
			if (isect.isOnSurface() || mQueryBeamType == SHORT_BEAM)
				contrib = mBB1DPhotonBeams.evalBeamBeamEstimate(mQueryBeamType, ray, mVolumeSegments, estimatorTechniques, originInMedium ? kOriginInMedium : 0, &data);
//			else
//				contrib = mBB1DPhotonBeams.evalBeamBeamEstimate(mQueryBeamType, ray, mLiteVolumeSegments, estimatorTechniques, originInMedium ? kOriginInMedium : 0, &data);
			const vec3 mult = cameraState.mThroughput * mBB1DNormalization;
			res.addColor(mult * contrib);
		}

		// Attenuate by intersected media (if any)
		float raySamplePdf(1.f);
		float raySampleRevPdf(1.f);
		if (!mVolumeSegments.empty()) {
			// PDF
			raySamplePdf = VolumeSegment::AccumulatePdf(mVolumeSegments);
			assert(raySamplePdf > 0.f);

			// Reverse PDF
			raySampleRevPdf = VolumeSegment::AccumulateRevPdf(mVolumeSegments);
			assert(raySampleRevPdf > 0.f);

			// Attenuation
			cameraState.mThroughput *= VolumeSegment::AccumulateAttenuationWithoutPdf(mVolumeSegments) / raySamplePdf;
		}

		if (isBlack(cameraState.mThroughput))
			break;

		// Prepare scattering function at the hitpoint (BSDF/phase depending on whether the hitpoint is at surface or in media, the isect knows)
		BSDF bsdf;
		if (isect.mFloor) {
			if (!bsdf.setupFloor(ctx, isect, 1.f, m_floor, false))
				break;

			// save ray state for environment lighting
			ctx.floorRayOrigin = ray.origin();
			ctx.floorRayDirection = ray.direction();
		} else {
			if (!bsdf.setup(ctx, isect, cameraState.mBoundaryStack, false))
				break;
		}

		if (cameraState.mPathLength == 1)
			res.albedo = bsdf.albedo();

		const vec3 hitPoint = ray.hitPoint;
		const bool bsdfEmission = !isBlack(bsdf.emission());
		const bool isOnLightSource = isect.mLight != nullptr || (bsdfEmission && isect.mGeometry->isLightSource());

		originInMedium = isect.isInMedium();

		// Update affected MIS data
		{
			const float distSq = sqr(isect.mDist);
			const float raySamplePdfInv = 1.f / raySamplePdf;
			mCameraVerticesMisData[cameraState.mPathLength].mPdfAInv = cameraState.mLastPdfWInv * distSq * raySamplePdfInv / std::fabs(bsdf.cosThetaFix());
			mCameraVerticesMisData[cameraState.mPathLength].mRevPdfA = 1.f;
			mCameraVerticesMisData[cameraState.mPathLength].mRaySamplePdfInv = raySamplePdfInv;
			mCameraVerticesMisData[cameraState.mPathLength].mRaySampleRevPdfInv = 1.f;
			mCameraVerticesMisData[cameraState.mPathLength].mSinTheta = 0.f;
			mCameraVerticesMisData[cameraState.mPathLength].mCosThetaOut = 0.f;
			mCameraVerticesMisData[cameraState.mPathLength].mSurfMisWeightFactor = bsdf.isOnSurface() ? (isOnLightSource ? 0.f : mSurfMisWeightFactor) : 0.f;
			mCameraVerticesMisData[cameraState.mPathLength].mPP3DMisWeightFactor = bsdf.isOnSurface() ? 0.f : mPP3DMisWeightFactor;
			mCameraVerticesMisData[cameraState.mPathLength].mPB2DMisWeightFactor = bsdf.isOnSurface() ? 0.f : mPB2DMisWeightFactor;
			mCameraVerticesMisData[cameraState.mPathLength].mBB1DMisWeightFactor = bsdf.isOnSurface() ? 0.f : mBB1DMisWeightFactor;
			mCameraVerticesMisData[cameraState.mPathLength].mBB1DBeamSelectionPdf = bsdf.isOnSurface() ? 0.f : ((mMergeWithLightVerticesBB1D && mPhotonBeamsCount > 0 && mBB1DPhotonBeams.sMaxBeamsInCell) ? mBB1DPhotonBeams.getBeamSelectionPdf(hitPoint) : 1.f);
			mCameraVerticesMisData[cameraState.mPathLength].mIsDelta = isOnLightSource ? false : bsdf.isDelta();
			mCameraVerticesMisData[cameraState.mPathLength].mIsOnLightSource = isOnLightSource;
			mCameraVerticesMisData[cameraState.mPathLength].mIsSpecular = false;
			mCameraVerticesMisData[cameraState.mPathLength].mInMediumWithBeams = bsdf.isOnSurface() ? false : (!mMergeWithLightVerticesPB2D || bsdf.medium()->getMeanFreePath(hitPoint) > mBB1DMinMFP);

			mCameraVerticesMisData[cameraState.mPathLength].mRaySamplePdfsRatio = 0.f;
			mCameraVerticesMisData[cameraState.mPathLength].mRaySampleRevPdfsRatio = 0.f;
			if (bsdf.isInMedium()) {
//				if (bsdf.medium()->isHomogeneous())
				{
					mCameraVerticesMisData[cameraState.mPathLength].mRaySamplePdfsRatio = 1.f / bsdf.medium()->mMinPositiveAttenuationCoefComp();
					mCameraVerticesMisData[cameraState.mPathLength].mRaySampleRevPdfsRatio = mCameraVerticesMisData[cameraState.mPathLength].mRaySamplePdfsRatio;
				}
//				else
//				{
//					const float lastSegmentRayOverSamplePdf = bsdf.medium()->RaySamplePdf(mVolumeSegments.back().mDistMin, mVolumeSegments.back().mDistMax, 0);
//					const float lastSegmentRayInSamplePdf = mVolumeSegments.back().mRaySamplePdf; // We are in medium -> we know we have insampled
//					mCameraVerticesMisData[cameraState.mPathLength].mRaySamplePdfsRatio = lastSegmentRayOverSamplePdf / lastSegmentRayInSamplePdf;
//				}
			}

			// Update reverse PDFs of the previous vertex
			mCameraVerticesMisData[cameraState.mPathLength - 1].mRevPdfA *= raySampleRevPdf / distSq;
			mCameraVerticesMisData[cameraState.mPathLength - 1].mRaySampleRevPdfInv = 1.f / raySampleRevPdf;

			if (lastMedium && !lastMedium->isHomogeneous()) { // Homogeneous case was solved immediately when processing the vertex for the first time
				float firstSegmentRayOverSampleRevPdf;
				lastMedium->raySamplePdf(mVolumeSegments.front().mDistMin, mVolumeSegments.front().mDistMax, 0, &firstSegmentRayOverSampleRevPdf);
				const float firstSegmentRayInSampleRevPdf = mVolumeSegments.front().mRaySampleRevPdf; // We were in medium -> we know we have insampled
				mCameraVerticesMisData[cameraState.mPathLength - 1].mRaySampleRevPdfsRatio = firstSegmentRayOverSampleRevPdf / firstSegmentRayInSampleRevPdf;
			}
		}

		// Light source has been hit; terminate afterwards, since
		// our light sources do not have reflective properties
		if (isect.mLight) {
			// We cannot end yet
//			if (cameraState.mPathLength < mMinPathLength)
//				break;

			// Stop if we are in the light sampling mode and could have sampled this light last time in the next event estimation
			if (mAlgorithm == kPTls && cameraState.mPathLength > 1 && !cameraState.mLastSpecular)
				break;

			// Add its contribution
			const vec3 contrib = cameraState.mThroughput * getLightRadiance(isect.mLight, cameraState, ray);
			res.addColor(contrib);
			break;
		}

		if (isOnLightSource) { // material emission
			const vec3 contrib = cameraState.mThroughput * getLightRadiance(isect.mGeometry, cameraState, ray, &bsdf.emission());
			res.addColor(contrib);

			mCameraVerticesMisData[cameraState.mPathLength].mSurfMisWeightFactor = bsdf.isOnSurface() ? mSurfMisWeightFactor : 0.f;
			mCameraVerticesMisData[cameraState.mPathLength].mIsDelta = bsdf.isDelta();
			mCameraVerticesMisData[cameraState.mPathLength].mIsOnLightSource = false;
		} else if (bsdfEmission) {
			const auto contrib = cameraState.mThroughput * bsdf.emission();
			res.addColor(contrib);
		}

		// Terminate if eye sub-path is too long for connections or merging
		if (cameraState.mPathLength >= m_maxPathLength)
			break;

		// Ignore contribution of primary rays from medium too close to camera
		if (cameraState.mPathLength > 1 || bsdf.isOnSurface() || isect.mDist >= mMinDistToMed) {
			////////////////////////////////////////////////////////////////
			// Vertex connection: Connect to a light source
			if (mConnectToLightSource && !bsdf.isDelta() && m_lightCount > 0 && (bsdf.isInMedium() || !onlySpecSurf)) {
				res.addColor(cameraState.mThroughput * directIllumination(cameraState, hitPoint, bsdf, ctx));
			}

			////////////////////////////////////////////////////////////////
			// Vertex connection: Connect to light vertices
			if (mConnectToLightVertices && !bsdf.isDelta() && mLightVertexCount > 0 && (bsdf.isInMedium() || !onlySpecSurf)) {
				// Determine whether the vertex is in medium behind real geometry
				bool behindSurf = false;
				if (bsdf.isInMedium() && !cameraState.mBoundaryStack.IsEmpty()) {
					auto mat = cameraState.mBoundaryStack.Top().mMedium;
					if (mat) {
//						if (mat->mGeometryType != GeometryType::IMAGINARY)
							behindSurf = true;
					}
				}

				int pathCountL = mPathCountPerIter;
				//int pathIdxMod = ctx.sampler.uiRand() % pathCountL;
				int pathIdxMod = std::min((int)(ctx.sampler.generate1D() * pathCountL), pathCountL - 1);

				// For VC, each light sub-path is assigned to a particular eye
				// sub-path, as in traditional BPT. It is also possible to
				// connect to vertices from any light path, but MIS should
				// be revisited.
				const auto & range = mPathSegments[pathIdxMod];
				for (int i = range.begin; i < range.end; i++) {
					const UPBPLightVertex &lightVertex = mLightVertices[i];

//					if (lightVertex.mPathLength + 1 + cameraState.mPathLength < mMinPathLength)
//						continue;

					// Light vertices are stored in increasing path length
					// order; once we go above the max path length, we can
					// skip the rest
					if (lightVertex.mPathLength + 1 +
						cameraState.mPathLength > m_maxPathLength)
						break;

					// We store all light vertices in order to compute MIS weights but not all can be used for VC
					if (!lightVertex.mConnectable)
						continue;

					// Don't try connect vertices in different media with real geometry
					if (lightVertex.mBSDF.isInMedium() && bsdf.isInMedium() && lightVertex.mBSDF.medium() != bsdf.medium()
						&& (lightVertex.mBehindSurf || behindSurf))
						continue;

					const vec3 mult = cameraState.mThroughput * lightVertex.mThroughput;
					res.addColor(mult * connectVertices(lightVertex, bsdf, hitPoint, cameraState, ctx));
				}
			}

			////////////////////////////////////////////////////////////////
			// Vertex merging: surface photon mapping
			if (mMergeWithLightVerticesSurf && bsdf.isOnSurface() && !bsdf.isDelta() && mLightVerticesOnSurfaceCount > 0 && !onlySpecSurf) {
				RangeQuery query(*this, hitPoint, bsdf, cameraState);
				mSurfHashGrid.process(mLightVertices.data(), query);
				const vec3 mult = cameraState.mThroughput * mSurfNormalization;
				res.addColor(mult * query.GetContrib());

				// PPM merges only at the first non-specular surface from camera
				if (mAlgorithm == kPPM) break;
			}

			////////////////////////////////////////////////////////////////
			// Vertex merging: point x point 3D
			if (mMergeWithLightVerticesPP3D && bsdf.isInMedium() && !bsdf.isDelta() && mLightVerticesInMediumCount > 0) {
				RangeQuery query(*this, hitPoint, bsdf, cameraState);
				mPP3DHashGrid.process(mLightVertices.data(), query);
				const vec3 mult = cameraState.mThroughput * mPP3DNormalization;
				res.addColor(mult * query.GetContrib());
			}
		}

		// Continue random walk
		if (!sampleScattering(bsdf, hitPoint, isect, cameraState,
							  mCameraVerticesMisData[cameraState.mPathLength], mCameraVerticesMisData[cameraState.mPathLength - 1], ctx))
			break;

		if (bsdf.isOnSurface()) {
			if (!cameraState.mLastSpecular) {
				if (onlySpecSurf)
					break;

				if (mEstimatorTechniques & BB1D_PREVIOUS)
					stopBB1D = true;
			}

			lastMedium = nullptr;
		} else {
			if (onlySpecSurf) {
				if (mEstimatorTechniques & COMPATIBLE)
					onlySpecSurf = false;
				else
					break;
			}

			if (mEstimatorTechniques & BB1D_PREVIOUS)
				stopBB1D = true;

			lastMedium = bsdf.medium();
		}

		// Prepare ray
		ray.setOrigin(cameraState.mOrigin);
		ray.setDirection(cameraState.mDirection);
		ray.ray.tnear = 0.f;
		ray.ray.tfar = m_rayLength;
	}
}

void UPBP::endIteration()
{
	for (auto & photons : mThreadCameraPhotons) {
		if (!m_interrupted) {
			for (auto & p : photons) {
				assert(isfinite(p.color.r) && isfinite(p.color.g) && isfinite(p.color.b));
				p.color = glm::min(p.color, m_maxIntensity);
				auto color = m_frameBuffer->getPixel(p.x, p.y);
				(vec3 &)color += p.color * m_frameBlend;
				m_frameBuffer->setPixel(p.x, p.y, color);
			}
		}

		photons.clear();
	}

	// Delete stored photons
	if (mMergeWithLightVerticesPB2D && m_maxPathLength > 1 && mLightVertexCount > 0) {
		mPB2DEmbreeBre.Destroy();
	}

	// Delete stored photon beams
	if (mMergeWithLightVerticesBB1D && m_maxPathLength > 1 && mPhotonBeamsCount > 0) {
		mBB1DPhotonBeams.destroy();
	}

	BaseRenderer::endIteration();
}

//////////////////////////////////////////////////////////////////////////
// Camera tracing methods
//////////////////////////////////////////////////////////////////////////

// Generates new camera sample given a pixel index
void UPBP::generateCameraSample(const TraceContext & aCtx, SubPathState & oCameraState)
{
	const Camera & camera = m_scene.camera();
	const Ray & primaryRay = aCtx.ray;
	auto cameraDirection = -glm::axisZ(camera.matrix());

	// Compute PDF conversion factor from area on image plane to solid angle on ray
	const float cosAtCamera = glm::dot(cameraDirection, primaryRay.direction());
	const float imagePointToCameraDist = mImagePlaneDist / cosAtCamera;
	const float imageToSolidAngleFactor = sqr(imagePointToCameraDist) / cosAtCamera;

	// We put the virtual image plane at such a distance from the camera origin
	// that the pixel area is one and thus the image plane sampling PDF is 1.
	// The solid angle ray PDF is then equal to the conversion factor from
	// image plane area density to ray solid angle density
	const float cameraPdfW = imageToSolidAngleFactor;

	oCameraState.mOrigin = primaryRay.origin();
	oCameraState.mDirection = primaryRay.direction();
	oCameraState.mThroughput = vec3(1.f);
	oCameraState.mPathLength = 1;
	oCameraState.mSpecularPath = 1;
	oCameraState.mLastSpecular = true;
	oCameraState.mLastPdfWInv = mScreenPixelCount / cameraPdfW;

	// Init the boundary stack with the global medium and add enclosing material and medium if present
	initBoundaryStack(oCameraState.mBoundaryStack);
	//	if (camera.mMatID != -1 && camera.mMedID != -1)
//		mScene.AddToBoundaryStack(camera.mMatID, camera.mMedID, oCameraState.mBoundaryStack);
}

// Returns the radiance of a light source when hit by a random ray,
// multiplied by MIS weight. Can be used for both Background and Area lights.
vec3 UPBP::getLightRadiance(const AbstractLight *aLight,
							const SubPathState  &aCameraState,
							const Ray           &aRay,
							const vec3			*aRadiance) const
{
	// We sample lights uniformly
	float directPdfA, emissionPdfW;
	vec3 radiance = aLight->getRadiance(aRay, &directPdfA, &emissionPdfW);
	if (aRadiance) radiance = *aRadiance;

	if (isBlack(radiance))
		return vec3(0.f);

	// If we see light source directly from camera, no weighting is required
	if (aCameraState.mPathLength == 1) {
		return radiance;
	}

	// When using only vertex merging, we want purely specular paths
	// to give radiance (cannot get it otherwise). Rest is handled
	// by merging and we should return 0.
	if (mEstimatorTechniques && !(mEstimatorTechniques & BPT))
		return aCameraState.mSpecularPath ? radiance : vec3(0.f);

	directPdfA *= m_lightPickProbability;
	emissionPdfW *= m_lightPickProbability;

	assert(directPdfA > 0.f);
	assert(emissionPdfW > 0.f);

	// MIS weight
	float misWeight = 1.f;
	if (mConnectToLightVertices) {
		assert(directPdfA > 0.f);
		const float wCamera = accumulateCameraPathWeight2(aCameraState.mPathLength, directPdfA, 0, 0, 0, emissionPdfW / directPdfA, aCameraState.mCameraVerticesMisData);
		misWeight = isfinite(wCamera) ? 1.f / (wCamera + 1.f) : 0.f;
	} else if (mAlgorithm == kPTmis && !aCameraState.mLastSpecular) {
		const float wCamera = directPdfA * aCameraState.mCameraVerticesMisData[aCameraState.mPathLength].mPdfAInv;
		assert(isfinite(wCamera));
		misWeight = 1.f / (wCamera + 1.f);
	}

	return misWeight * radiance;
}

// Connects camera vertex to randomly chosen light point.
// Returns emitted radiance multiplied by path MIS weight.
// Has to be called AFTER updating the MIS quantities.
vec3 UPBP::directIllumination(const SubPathState  &aCameraState,
							  const vec3          &aHitpoint,
							  const BSDF          &aCameraBSDF,
							  const TraceContext &aCtx)
{
	// We sample lights uniformly
	const auto	lightID = size_t(aCtx.sampler.generate1D() * m_lightCount);
	auto light = getLight(lightID);
	assert(light);

	// Light in infinity in attenuating homogeneous global medium is always reduced to zero
//	if (!light->isFinite() && mScene.GetGlobalMediumPtr()->HasAttenuation())
//		return vec3(0.f);

	vec3 directionToLight;
	float distance;
	float directPdfW, emissionPdfW, cosAtLight;
	const vec3 radiance = light->illuminate(aHitpoint,
											aCtx.sampler, directionToLight, distance, directPdfW,
											&emissionPdfW, &cosAtLight);

	// If radiance == 0, other values are undefined, so shave to early exit
	if (isBlack(radiance))
		return vec3(0.f);

	assert(directPdfW > 0.f);
	assert(emissionPdfW > 0.f);
	assert(cosAtLight > 0.f);

	// Get BSDF factor at the last camera vertex
	float bsdfDirPdfW, bsdfRevPdfW, cosToLight, sinTheta;
	vec3 bsdfFactor = aCameraBSDF.evaluate(directionToLight, cosToLight, &bsdfDirPdfW, &bsdfRevPdfW, &sinTheta);

	if (isBlack(bsdfFactor))
		return vec3(0.f);

	const float continuationProbability = aCameraBSDF.continuationProbability();

	// If the light is delta light, we can never hit it
	// by BSDF sampling, so the probability of this path is 0
	bsdfDirPdfW *= light->isDelta() ? 0.f : continuationProbability;
	bsdfRevPdfW *= continuationProbability;

	assert(bsdfRevPdfW > 0.f);
	assert(cosToLight > 0.f);

	vec3 contrib(0.f);

	// Test occlusion
	VolumeSegments	mVolumeSegments;
	if (!occluded(aHitpoint, directionToLight, distance,
				  aCtx.ray, aCtx.sampler,
				  aCameraState.mBoundaryStack, aCameraBSDF.isInMedium() ? kOriginInMedium : 0, mVolumeSegments)) {
		// Get attenuation from intersected media (if any)
		float nextRaySamplePdf(1.f);
		float nextRaySampleRevPdf(1.f);
		vec3 nextAttenuation(1.f);
		if (!mVolumeSegments.empty()) {
			// PDF
			nextRaySamplePdf = VolumeSegment::AccumulatePdf(mVolumeSegments);
			assert(nextRaySamplePdf > 0.f);

			// Reverse PDF
			nextRaySampleRevPdf = VolumeSegment::AccumulateRevPdf(mVolumeSegments);
			assert(nextRaySampleRevPdf > 0.f);

			// Attenuation (without PDF!)
			nextAttenuation = VolumeSegment::AccumulateAttenuationWithoutPdf(mVolumeSegments);
			if (!isPositive(nextAttenuation))
				return vec3(0.f);
		}

		// MIS weight
		float misWeight = 1.f;
		if (mConnectToLightVertices) {
			float lastSinTheta = 0;
			float lastRaySampleRevPdfInv = 0;
			float lastRaySampleRevPdfsRatio = 0;
			if (aCameraBSDF.isInMedium()) {
				lastSinTheta = sinTheta;
				lastRaySampleRevPdfInv = 1.f / nextRaySampleRevPdf;
				lastRaySampleRevPdfsRatio = aCameraState.mCameraVerticesMisData[aCameraState.mPathLength].mRaySamplePdfsRatio;
				if (!aCameraBSDF.medium()->isHomogeneous()) {
					float firstSegmentRayOverSampleRevPdf;
					aCameraBSDF.medium()->raySamplePdf(mVolumeSegments.front().mDistMin, mVolumeSegments.front().mDistMax, 0, &firstSegmentRayOverSampleRevPdf);
					const float firstSegmentRayInSampleRevPdf = mVolumeSegments.front().mRaySampleRevPdf; // We are in medium -> we know we have insampled
					lastRaySampleRevPdfsRatio = firstSegmentRayOverSampleRevPdf / firstSegmentRayInSampleRevPdf;
				}
			}

			// For wCamera we need ratio = emissionPdfA / directPdfA,
			// with emissionPdfA being the product of the PDFs for choosing the
			// point on the light source and sampling the outgoing direction.
			// What we are given by the light source instead are emissionPdfW
			// and directPdfW. Converting to area PDFs and plugging into ratio:
			//    emissionPdfA = emissionPdfW * cosToLight / dist^2
			//    directPdfA   = directPdfW * cosAtLight / dist^2
			//    ratio = (emissionPdfW * cosToLight / dist^2) / (directPdfW * cosAtLight / dist^2)
			//    ratio = (emissionPdfW * cosToLight) / (directPdfW * cosAtLight)
			// Also note that both emissionPdfW and directPdfW should be
			// multiplied by lightPickProb, so it cancels out.
			assert(nextRaySampleRevPdf * emissionPdfW * cosToLight / (directPdfW * cosAtLight) > 0.f);
			const float wCamera = accumulateCameraPathWeight2(aCameraState.mPathLength, nextRaySampleRevPdf * emissionPdfW * cosToLight / (directPdfW * cosAtLight), lastSinTheta, lastRaySampleRevPdfInv, lastRaySampleRevPdfsRatio, bsdfRevPdfW, aCameraState.mCameraVerticesMisData);

			// Note that wLight is a ratio of area PDFs. But since both are on the
			// light source, their distance^2 and cosine terms cancel out.
			// Therefore we can write wLight as a ratio of solid angle PDFs,
			// both expressed w.r.t. the same shading point.
			const float wLight = light->isDelta() ? 0 : (nextRaySamplePdf * bsdfDirPdfW) / (directPdfW * m_lightPickProbability);
			misWeight = 1.f / (wCamera + 1.f + wLight);
		} else if (mAlgorithm != kPTls && !light->isDelta())
			misWeight = mis2(m_lightPickProbability * directPdfW, bsdfDirPdfW * nextRaySamplePdf);

		contrib = (cosToLight / (m_lightPickProbability * directPdfW)) * (radiance * nextAttenuation * bsdfFactor);
		contrib *= misWeight;
	}

	if (isBlack(contrib)) {
		return vec3(0.f);
	}

	return contrib;
}

// Connects an eye and a light vertex. Result multiplied by MIS weight, but
// not multiplied by vertex throughputs. Has to be called AFTER updating MIS
// constants. 'direction' is FROM eye TO light vertex.
vec3 UPBP::connectVertices(const UPBPLightVertex	&aLightVertex,
						   const BSDF				&aCameraBSDF,
						   const vec3				&aCameraHitpoint,
						   const SubPathState		&aCameraState,
						   const TraceContext		&aCtx)
{
	// Get the connection
	vec3 direction = aLightVertex.mHitpoint - aCameraHitpoint;
	const float dist2 = glm::length2(direction);
	float  distance = std::sqrt(dist2);
	direction /= distance;

	// Evaluate BSDF at camera vertex
	float cosCamera, cameraBsdfDirPdfW, cameraBsdfRevPdfW, sinThetaCamera;
	vec3 cameraBsdfFactor = aCameraBSDF.evaluate(direction, cosCamera, &cameraBsdfDirPdfW,
												 &cameraBsdfRevPdfW, &sinThetaCamera);

	if (isBlack(cameraBsdfFactor))
		return vec3(0.f);

	// Camera continuation probability (for Russian roulette)
	const float cameraCont = aCameraBSDF.continuationProbability();
	cameraBsdfDirPdfW *= cameraCont;
	cameraBsdfRevPdfW *= cameraCont;
	assert(cameraBsdfDirPdfW > 0.f);
	assert(cameraBsdfRevPdfW > 0.f);

	// Evaluate BSDF at light vertex
	float cosLight, lightBsdfDirPdfW, lightBsdfRevPdfW, sinThetaLight;
	const vec3 lightBsdfFactor = aLightVertex.mBSDF.evaluate(-direction, cosLight, &lightBsdfDirPdfW,
															 &lightBsdfRevPdfW, &sinThetaLight);

	if (isBlack(lightBsdfFactor))
		return vec3(0.f);

	// Light continuation probability (for Russian roulette)
	const float lightCont = aLightVertex.mBSDF.continuationProbability();
	lightBsdfDirPdfW *= lightCont;
	lightBsdfRevPdfW *= lightCont;
	assert(lightBsdfDirPdfW > 0.f);
	assert(lightBsdfRevPdfW > 0.f);

	// Compute geometry term
	const float geometryTerm = cosLight * cosCamera / dist2;
	if (geometryTerm < 0.f)
		return vec3(0.f);

	// Convert PDFs to area PDF
	const float cameraBsdfDirPdfA = pdfWtoA(cameraBsdfDirPdfW, distance, cosLight);
	const float lightBsdfDirPdfA = pdfWtoA(lightBsdfDirPdfW, distance, cosCamera);
	assert(cameraBsdfDirPdfA > 0.f);
	assert(lightBsdfDirPdfA > 0.f);

	uint raySamplingFlags = 0;
	if (aCameraBSDF.isInMedium()) raySamplingFlags |= kOriginInMedium;
	if (aLightVertex.mInMedium)   raySamplingFlags |= kEndInMedium;

	// Test occlusion
	VolumeSegments	mVolumeSegments;
	if (occluded(aCameraHitpoint, direction, distance,
				 aCtx.ray, aCtx.sampler,
				 aCameraState.mBoundaryStack, raySamplingFlags, mVolumeSegments))
		return vec3(0.f);

	// Attenuate by intersected media (if any)
	float raySamplePdf(1.f);
	float raySampleRevPdf(1.f);
	vec3 mediaAttenuation(1.f);
	if (!mVolumeSegments.empty()) {
		// PDF
		raySamplePdf = VolumeSegment::AccumulatePdf(mVolumeSegments);
		assert(raySamplePdf > 0.f);

		// Reverse PDF
		raySampleRevPdf = VolumeSegment::AccumulateRevPdf(mVolumeSegments);
		assert(raySampleRevPdf > 0.f);

		// Attenuation (without PDF!)
		mediaAttenuation = VolumeSegment::AccumulateAttenuationWithoutPdf(mVolumeSegments);
		if (!isPositive(mediaAttenuation))
			return vec3(0.f);
	}

	// MIS weight

	// Camera part
	float lastSinThetaCamera = 0;
	float lastRaySampleRevPdfInvCamera = 0;
	float lastRaySampleRevPdfsRatioCamera = 0;
	if (aCameraBSDF.isInMedium()) {
		lastSinThetaCamera = sinThetaCamera;
		lastRaySampleRevPdfInvCamera = 1.f / raySampleRevPdf;
		lastRaySampleRevPdfsRatioCamera = aCameraState.mCameraVerticesMisData[aCameraState.mPathLength].mRaySamplePdfsRatio;
		if (!aCameraBSDF.medium()->isHomogeneous()) {
			float firstSegmentRayOverSampleRevPdf;
			aCameraBSDF.medium()->raySamplePdf(mVolumeSegments.front().mDistMin, mVolumeSegments.front().mDistMax, 0, &firstSegmentRayOverSampleRevPdf);
			const float firstSegmentRayInSampleRevPdf = mVolumeSegments.front().mRaySampleRevPdf; // We are in medium -> we know we have insampled
			lastRaySampleRevPdfsRatioCamera = firstSegmentRayOverSampleRevPdf / firstSegmentRayInSampleRevPdf;
		}
	}
	assert(raySampleRevPdf * lightBsdfDirPdfA > 0.f);
	const float wCamera = accumulateCameraPathWeight2(aCameraState.mPathLength, raySampleRevPdf * lightBsdfDirPdfA, lastSinThetaCamera, lastRaySampleRevPdfInvCamera, lastRaySampleRevPdfsRatioCamera, cameraBsdfRevPdfW, aCameraState.mCameraVerticesMisData);

	// Light part
	float lastSinThetaLight = 0;
	float lastRaySampleRevPdfInvLight = 0;
	float lastRaySampleRevPdfsRatioLight = 0;
	if (aLightVertex.mInMedium) {
		lastSinThetaLight = sinThetaLight;
		lastRaySampleRevPdfInvLight = 1.f / raySamplePdf;
		lastRaySampleRevPdfsRatioLight = aLightVertex.mMisData.mRaySamplePdfsRatio;
		if (!aLightVertex.mBSDF.medium()->isHomogeneous()) {
			const float lastSegmentRayOverSamplePdf = aLightVertex.mBSDF.medium()->raySamplePdf(mVolumeSegments.back().mDistMin, mVolumeSegments.back().mDistMax, 0);
			const float lastSegmentRayInSamplePdf = mVolumeSegments.back().mRaySamplePdf; // We are in medium -> we know we have insampled
			lastRaySampleRevPdfsRatioLight = lastSegmentRayOverSamplePdf / lastSegmentRayInSamplePdf;
		}
	}
	assert(raySamplePdf * cameraBsdfDirPdfA > 0.f);
	const float wLight = accumulateLightPathWeight2(aLightVertex.mPathIdx, aLightVertex.mPathLength, raySamplePdf * cameraBsdfDirPdfA, lastSinThetaLight, lastRaySampleRevPdfInvLight, lastRaySampleRevPdfsRatioLight, lightBsdfRevPdfW, BPT, false);
	const float misWeight = 1.f / (wCamera + 1.f + wLight);

	vec3 contrib = (geometryTerm) * cameraBsdfFactor * lightBsdfFactor * mediaAttenuation;
	contrib *= misWeight;

	return contrib;
}

//////////////////////////////////////////////////////////////////////////
// Light tracing methods
//////////////////////////////////////////////////////////////////////////

// Samples light emission
bool UPBP::generateLightSample(int aPathIdx, SubPathState &oLightState, std::vector<UPBPLightVertex> & oLightVertices, Sampler & sampler)
{
	// We sample lights uniformly
	const auto	lightID = size_t(sampler.generate1D() * m_lightCount);
	auto light = getLight(lightID);
	assert(light);

	float emissionPdfW, directPdfA, cosLight;
	oLightState.mThroughput = light->emitParticle(sampler, oLightState.mOrigin, oLightState.mDirection, emissionPdfW, &directPdfA, &cosLight);

	if (isBlack(oLightState.mThroughput))
		return false;

	emissionPdfW *= m_lightPickProbability;
	directPdfA *= m_lightPickProbability;

	assert(emissionPdfW);
	assert(directPdfA);

	// Store vertex

	UPBPLightVertex lightVertex;
	lightVertex.mHitpoint = oLightState.mOrigin;
	lightVertex.mThroughput = vec3(1.f);
	lightVertex.mPathLength = 0;
	lightVertex.mPathIdx = aPathIdx;
	lightVertex.mInMedium = false;
	lightVertex.mConnectable = false;
	lightVertex.mIsFinite = light->isFinite();

	lightVertex.mMisData.mPdfAInv = 1.f / directPdfA;
	lightVertex.mMisData.mRevPdfA = light->isDelta() ? 0.f : (light->isFinite() ? cosLight : 1.f);
	lightVertex.mMisData.mRevPdfAWithoutBsdf = lightVertex.mMisData.mRevPdfA;
	lightVertex.mMisData.mRaySamplePdfInv = 0.f;
	lightVertex.mMisData.mRaySampleRevPdfInv = 1.f;
	lightVertex.mMisData.mRaySamplePdfsRatio = 0.f;
	lightVertex.mMisData.mRaySampleRevPdfsRatio = 0.f;
	lightVertex.mMisData.mSinTheta = 0.f;
	lightVertex.mMisData.mCosThetaOut = (!light->isDelta() && light->isFinite()) ? cosLight : 1.f;
	lightVertex.mMisData.mSurfMisWeightFactor = 0.f;
	lightVertex.mMisData.mPP3DMisWeightFactor = 0.f;
	lightVertex.mMisData.mPB2DMisWeightFactor = 0.f;
	lightVertex.mMisData.mBB1DMisWeightFactor = 0.f;
	lightVertex.mMisData.mBB1DBeamSelectionPdf = 0.f;
	lightVertex.mMisData.mIsDelta = light->isDelta();
	lightVertex.mMisData.mIsOnLightSource = true;
	lightVertex.mMisData.mIsSpecular = false;
	lightVertex.mMisData.mInMediumWithBeams = false;

	mLightVerticesOnSurfaceCount++;
	oLightVertices.emplace_back(lightVertex);

	// Complete light path state initialization

	oLightState.mThroughput /= emissionPdfW;
	oLightState.mPathLength = 1;
	oLightState.mIsFiniteLight = light->isFinite() ? 1 : 0;
	oLightState.mLastSpecular = false;
	oLightState.mLastPdfWInv = directPdfA / emissionPdfW;

	// Init the boundary stack with the global medium and add enclosing material and medium if present
	initBoundaryStack(oLightState.mBoundaryStack);
//	if (light->mMatID != -1 && light->mMedID != -1)
//		mScene.AddToBoundaryStack(light->mMatID, light->mMedID, oLightState.mBoundaryStack);

	return true;
}

// Computes contribution of light sample to camera by splatting is onto the
// framebuffer. Multiplies by throughput (obviously, as nothing is returned).
void UPBP::connectToCamera(const UPBPLightVertex	*aLightVertices,
						   const SubPathState		&aLightState,
						   const vec3				&aHitpoint,
						   const BSDF				&aLightBSDF,
						   const float				aRaySampleRevPdfsRatio,
						   const TraceContext		&aCtx,
						   CameraPhotonsArray		&oCameraPhotons)
{
	auto & camera = m_scene.camera();
	if (camera.projection().getIndex() != CP_PERSPECTIVE)
		return;

	auto directionToCamera = glm::translation(camera.matrix()) - aHitpoint;
	auto cameraDirection = -glm::axisZ(camera.matrix());

	// Check point is in front of camera
	if (glm::dot(cameraDirection, -directionToCamera) <= 0.f)
		return;

	vec2 v;
	if (!camera.rayProjector()->getViewportCoords(v, aHitpoint, true))
		return;

	auto x = (int)(v.x * m_viewportToScreen[0].x + m_viewportToScreen[1].x) - (float)m_renderArea.tilePos.x;
	auto y = (int)(v.y * m_viewportToScreen[0].y + m_viewportToScreen[1].y) - (float)m_renderArea.tilePos.y;
	if (x < 0 || y < 0 || x >= (float)m_renderArea.tileSize.x || y >= (float)m_renderArea.tileSize.y)
		return;

	// Compute distance and normalize direction to camera
	const float distEye2 = glm::length2(directionToCamera);
	const float distance = std::sqrt(distEye2);
	directionToCamera /= distance;

	// Ignore contribution of primary rays from medium too close to camera
	if (aLightBSDF.isInMedium() && distance < mMinDistToMed)
		return;

	// Get the BSDF factor
	float cosToCamera, bsdfDirPdfW, bsdfRevPdfW, sinTheta;
	vec3 bsdfFactor = aLightBSDF.evaluate(directionToCamera, cosToCamera, &bsdfDirPdfW, &bsdfRevPdfW, &sinTheta);

	if (isBlack(bsdfFactor))
		return;

	bsdfRevPdfW *= aLightBSDF.continuationProbability();

	assert(bsdfDirPdfW > 0.f);
	assert(bsdfRevPdfW > 0.f);
	assert(cosToCamera > 0.f);

	// Compute PDF conversion factor from image plane area to surface area
	const float cosAtCamera = glm::dot(cameraDirection, -directionToCamera);
	const float imagePointToCameraDist = mImagePlaneDist / cosAtCamera;
	const float imageToSolidAngleFactor = sqr(imagePointToCameraDist) / cosAtCamera;
	const float imageToSurfaceFactor = imageToSolidAngleFactor * std::fabs(cosToCamera) / sqr(distance);

	// We put the virtual image plane at such a distance from the camera origin
	// that the pixel area is one and thus the image plane sampling PDF is 1.
	// The area PDF of aHitpoint as sampled from the camera is then equal to
	// the conversion factor from image plane area density to surface area density
	const float cameraPdfA = imageToSurfaceFactor;
	assert(cameraPdfA > 0.f);

	const float surfaceToImageFactor = 1.f / imageToSurfaceFactor;

	// Test occlusion
	VolumeSegments	mVolumeSegments;
	if (!occluded(aHitpoint, directionToCamera, distance,
				  aCtx.ray, aCtx.sampler,
				  aLightState.mBoundaryStack, aLightBSDF.isInMedium() ? kOriginInMedium : 0, mVolumeSegments)) {
		// Get attenuation from intersected media (if any)
		float raySampleRevPdf(1.f);
		vec3 mediaAttenuation(1.f);
		if (!mVolumeSegments.empty()) {
			// Reverse PDF
			raySampleRevPdf = VolumeSegment::AccumulateRevPdf(mVolumeSegments);
			assert(raySampleRevPdf > 0.f);

			// Attenuation (without PDF!)
			mediaAttenuation = VolumeSegment::AccumulateAttenuationWithoutPdf(mVolumeSegments);
			if (!isPositive(mediaAttenuation))
				return;
		}

		// Compute MIS weight if not doing LT
		float misWeight = 1.f;
		if (mAlgorithm != kLT) {
			float lastSinTheta = 0;
			float lastRaySampleRevPdfInv = 0;
			float lastRaySampleRevPdfsRatio = 0;
			if (aLightBSDF.isInMedium()) {
				lastSinTheta = sinTheta;
				lastRaySampleRevPdfInv = 1.f / raySampleRevPdf;
				lastRaySampleRevPdfsRatio = aRaySampleRevPdfsRatio;
				if (!aLightBSDF.medium()->isHomogeneous()) {
					float firstSegmentRayOverSampleRevPdf;
					aLightBSDF.medium()->raySamplePdf(mVolumeSegments.front().mDistMin, mVolumeSegments.front().mDistMax, 0, &firstSegmentRayOverSampleRevPdf);
					const float firstSegmentRayInSampleRevPdf = mVolumeSegments.front().mRaySampleRevPdf; // We are in medium -> we know we have insampled
					lastRaySampleRevPdfsRatio = firstSegmentRayOverSampleRevPdf / firstSegmentRayInSampleRevPdf;
				}
			}
			assert(raySampleRevPdf * cameraPdfA > 0.f);
			const float wLight = AccumulateLightPathWeight(aLightVertices, aLightState.mPathLength, raySampleRevPdf * cameraPdfA, lastSinTheta, lastRaySampleRevPdfInv, lastRaySampleRevPdfsRatio, bsdfRevPdfW, BPT, mQueryBeamType, mPhotonBeamType, mEstimatorTechniques, true) / mScreenPixelCount;
			misWeight = 1.f / (1.f + wLight);
		}

		// We divide the contribution by surfaceToImageFactor to convert the (already
		// divided) PDF from surface area to image plane area, w.r.t. which the
		// pixel integral is actually defined. We also divide by the number of samples
		// this technique makes, which is equal to the number of light sub-paths
		vec3 contrib = aLightState.mThroughput * bsdfFactor * mediaAttenuation / (mLightSubPathCount * surfaceToImageFactor);
		contrib *= misWeight;

		if (isBlack(contrib))
			return;

		oCameraPhotons.emplace_back(CameraPhoton{contrib, (uint16_t)x, (uint16_t)y});

//		auto color = m_renderImage->getPixel(x, y);
//		(vec3 &)color += contrib * m_frameBlend;
//		m_renderImage->setPixel(x, y, color);
	}
}

// Adds beams to beams array
void UPBP::addBeams(const Ray &aRay,
					const vec3 &aThroughput,
					const VolumeSegments & mVolumeSegments,
					UPBPLightVertex *aLightVertex,
					const uint aRaySamplingFlags,
					const float aLastPdfWInv,
					PhotonBeamsArray & oPhotonBeamsArray)
{
	assert(aRaySamplingFlags == 0 || aRaySamplingFlags == kOriginInMedium);
	assert(aLightVertex);

	vec3 throughput = aThroughput;
	float raySamplePdf = 1.f;
	float raySampleRevPdf = 1.f;

	if (mPhotonBeamType == SHORT_BEAM) {
		for (auto it = mVolumeSegments.cbegin(); it != mVolumeSegments.cend(); ++it) {
			assert(it->mMedium);
			PhotonBeam beam;
			beam.mAABB.clear();
			beam.mMedium = it->mMedium;
			if (beam.mMedium->hasScattering() && (!mMergeWithLightVerticesPB2D || beam.mMedium->getMeanFreePath(aRay.origin()) > mBB1DMinMFP)) {
				beam.mLength = it->mDistMax - it->mDistMin;
				beam.mRay.init(aRay.target(it->mDistMin), aRay.direction(), beam.mLength);
				beam.mFlags = SHORT_BEAM;
				beam.mRaySamplePdf = raySamplePdf;
				beam.mRaySampleRevPdf = raySampleRevPdf;
				beam.mRaySamplingFlags = kEndInMedium;
				if (it == mVolumeSegments.cbegin())
					beam.mRaySamplingFlags |= aRaySamplingFlags;
				beam.mLastPdfWInv = aLastPdfWInv;
				beam.mThroughputAtOrigin = throughput;
				beam.mLightVertex = aLightVertex;

				assert(oPhotonBeamsArray.size() < oPhotonBeamsArray.capacity());
				oPhotonBeamsArray.emplace_back(beam);
			}
			throughput *= it->mAttenuation / it->mRaySamplePdf;
			raySamplePdf *= it->mRaySamplePdf;
			raySampleRevPdf *= it->mRaySampleRevPdf;
		}
	}
//	else // LONG_BEAM
//	{
//		assert(mPhotonBeamType == LONG_BEAM);
//		for (auto it = mLiteVolumeSegments.cbegin(); it != mLiteVolumeSegments.cend(); ++it)
//		{
//			assert(it->mMediumID);
//			PhotonBeam beam;
//			beam.mMedium = mScene.mMedia[it->mMediumID];
//			if (beam.mMedium->HasScattering() && (!mMergeWithLightVerticesPB2D || beam.mMedium->GetMeanFreePath(aRay.origin) > mBB1DMinMFP))
//			{
//				beam.mLength = it->mDistMax - it->mDistMin;
//				beam.mRay = Ray(aRay.origin + aRay.direction * it->mDistMin, aRay.direction, beam.mLength);
//				beam.mFlags = LONG_BEAM;
//				beam.mRaySamplePdf = raySamplePdf;
//				beam.mRaySampleRevPdf = raySampleRevPdf;
//				beam.mRaySamplingFlags = kEndInMedium;
//				if (it == mLiteVolumeSegments.cbegin())
//					beam.mRaySamplingFlags |= aRaySamplingFlags;
//				beam.mLastPdfWInv = aLastPdfWInv;
//				beam.mThroughputAtOrigin = throughput;
//				beam.mLightVertex = aLightVertex;
//
//				assert(oPhotonBeamsArray.size() < oPhotonBeamsArray.capacity());
//				oPhotonBeamsArray.emplace_back(beam);
//			}
//			if (beam.mMedium->isHomogeneous())
//			{
//				const HomogeneousMedium * medium = ((const HomogeneousMedium *)beam.mMedium);
//				throughput *= medium->EvalAttenuation(it->mDistMax - it->mDistMin);
//			}
//			else
//			{
//				throughput *= beam.mMedium->EvalAttenuation(aRay, it->mDistMin, it->mDistMax);
//			}
//			float segmentRaySampleRevPdf;
//			float segmentRaySamplePdf = beam.mMedium->RaySamplePdf(it->mDistMin, it->mDistMax, it == mLiteVolumeSegments.cbegin() ? aRaySamplingFlags : 0, &segmentRaySampleRevPdf);
//			raySamplePdf *= segmentRaySamplePdf;
//			raySampleRevPdf *= segmentRaySampleRevPdf;
//		}
//
//		if (!oPhotonBeamsArray.empty() && oPhotonBeamsArray.back().mLength > oPhotonBeamsArray.back().mMedium->MaxBeamLength())
//			oPhotonBeamsArray.back().mLength = oPhotonBeamsArray.back().mMedium->MaxBeamLength();
//	}
}

//////////////////////////////////////////////////////////////////////////
// Common methods
//////////////////////////////////////////////////////////////////////////

// Samples a scattering direction camera/light sample according to BSDF.
// Returns false for termination
bool UPBP::sampleScattering(const BSDF		&aBSDF,
							const vec3		&aHitPoint,
							const Isect		&aIsect,
							SubPathState	&aoState,
							MisData			&aoCurrentMisData,
							MisData			&aoPreviousMisData,
							TraceContext	&ctx)
{
	// Sample scattering function

	vec3     rndTriplet = ctx.sampler.generate3D(); // x,y for direction, z for component. No rescaling happens
	float    bsdfDirPdfW, cosThetaOut, sinTheta;
	uint32_t sampledEvent;

	vec3 bsdfFactor = aBSDF.sample(ctx, rndTriplet, aoState.mDirection,
								   bsdfDirPdfW, cosThetaOut,
								   &sampledEvent, &sinTheta);

	if (isBlack(bsdfFactor) || bsdfDirPdfW <= 0.f)
		return false;

	bool specular = (sampledEvent & BSDF::kSpecular) != 0;

	// If we sampled specular event, then the reverse probability
	// cannot be evaluated, but we know it is exactly the same as
	// forward probability, so just set it. If non-specular event happened,
	// we evaluate the PDF
	float bsdfRevPdfW = bsdfDirPdfW;
	if (!specular)
		bsdfRevPdfW = aBSDF.pdf(aoState.mDirection, cosThetaOut, true);

	assert(bsdfDirPdfW > 0.f);
	assert(bsdfRevPdfW > 0.f);

	// Russian roulette
	const float contProb = aBSDF.continuationProbability();
	if (contProb == 0)
		return false;

	if (contProb < 1.f && (aBSDF.isFromLight() || aoState.mPathLength > m_russianRouletteStart)) {
		if (ctx.sampler.generate1D() > contProb)
			return false;

		bsdfDirPdfW *= contProb;
		bsdfRevPdfW *= contProb;
	}

	const float bsdfDirPdfWInv = 1.f / bsdfDirPdfW;

	// Update path state

	aoState.mOrigin = aHitPoint;
	aoState.mThroughput *= bsdfFactor * (cosThetaOut * bsdfDirPdfWInv);
	assert(isfinite(aoState.mThroughput.x));
	aoState.mSpecularPath &= specular ? 1 : 0;
	aoState.mLastPdfWInv = bsdfDirPdfWInv;
	aoState.mLastSpecular = specular;

	// Switch medium on refraction
	if ((sampledEvent & (BSDF::kRefract | BSDF::kTransmit)) != 0)
		updateBoundaryStackOnRefract(aIsect, aoState.mBoundaryStack);

	// Update affected MIS data
	aoCurrentMisData.mRevPdfA *= cosThetaOut;
	aoCurrentMisData.mRevPdfAWithoutBsdf = aoCurrentMisData.mRevPdfA;
	aoCurrentMisData.mIsSpecular = specular;
	aoCurrentMisData.mSinTheta = sinTheta;
	aoCurrentMisData.mCosThetaOut = cosThetaOut;
	aoPreviousMisData.mRevPdfA *= bsdfRevPdfW;

	return true;
}
