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

#ifndef __UPBPLIGHTVERTEX_HXX__
#define __UPBPLIGHTVERTEX_HXX__

#include "Defs.h"

struct MisData
{
	float mPdfAInv;               // Inverse of forward PDF
	float mRevPdfA;               // Reverse PDF
	float mRevPdfAWithoutBsdf;    // Reverse PDF without PDF of sampling BSDF, needed for next-to-last light vertices only (next-to-last camera vertices never have BSDF included since it is not yet known then)
	float mRaySamplePdfInv;       // Inverse of forward ray sampling probability, needed for PB2D (camera part) and BB1D (both)
	float mRaySampleRevPdfInv;    // Inverse of reverse ray sampling probability, needed for PB2D (light part) and BB1D (both)
	float mRaySamplePdfsRatio;    // Forward probability of sampling the medium behind the vertex divided by forward probability of sampling the medium at the vertex, needed for PB2D and BB1D, nonzero only for vertices in media
	float mRaySampleRevPdfsRatio; // Reverse probability of sampling the medium behind the vertex divided by reverse probability of sampling the medium at the vertex, needed for BB1D, nonzero only for vertices in media
	float mSinTheta;              // Sine of angle between incoming and outgoing direction of vertex in medium
	float mCosThetaOut;           // Cosine of angle between normal and outgoing direction
	float mSurfMisWeightFactor;   // Weight factor of SURF, nonzero only for vertices on surface
	float mPP3DMisWeightFactor;   // Weight factor of PP3D, nonzero only for vertices in media
	float mPB2DMisWeightFactor;   // Weight factor of PB2D, nonzero only for vertices in media
	float mBB1DMisWeightFactor;   // Weight factor of BB1D, nonzero only for vertices in media
	float mBB1DBeamSelectionPdf;  // Probability of selecting a beam (in case of beam reduction)
	bool  mIsDelta;               // Whether the vertex is on a delta surface
	bool  mIsOnLightSource;       // Whether the vertex is on a light source
	bool  mIsSpecular;            // Whether the event sampled at the vertex was specular
	bool  mInMediumWithBeams;     // Whether the vertex is in medium that allows storing beams
};

// Light vertex, used for upbp
struct UPBPLightVertex
{
	vec3  mHitpoint;   // Position of the vertex
	vec3  mThroughput; // Path throughput (including emission)
	int   mPathIdx;    // Path index
	uint  mPathLength; // Number of segments between source and vertex		
	bool  mInMedium;   // Vertex in participating medium, not on surface
	bool  mBehindSurf; // Vertex in participating medium behind real (not imaginary) surface
	bool  mConnectable;// Whether this vertex can be used in vertex connection
	bool  mIsFinite;   // Whether this vertex is not on infinite light source
	BSDF  mBSDF;       // Stores all required local information, including incoming direction

	MisData mMisData;  // Data needed for MIS weights computation

	// Used by HashGrid
	const vec3 & GetPosition() const
	{
		return mHitpoint;
	}

	bool MatchesType(uint aType) const
	{
		return (mInMedium && aType == PP3D) || (!mInMedium && aType == SURF);
	}
};

struct PathSegment
{
	int begin;
	int end;
};

struct MisData;
class PhotonBeamsEvaluator;

struct AdditionalRayDataForMis
{
	const UPBPLightVertex *mLightVertices;
	const PathSegment     *mPathSegments;
	MisData               *mCameraVerticesMisData;
	const unsigned int    mCameraPathLength;
	const unsigned int    mMinPathLength;
	const unsigned int    mMaxPathLength;
	const BeamType        mQueryBeamType;
	const BeamType        mPhotonBeamType;
	const float           mLastPdfWInv;
	const float           mSurfMisWeightFactor;
	const float           mPP3DMisWeightFactor;
	const float           mPB2DMisWeightFactor;
	const float           mBB1DMisWeightFactor;
	PhotonBeamsEvaluator  *mBB1DPhotonBeams;
	const float           mBB1DMinMFP;
	const float           mLightSubPathCount;
	const float           mMinDistToMed;
	float                 mRaySamplePdf;
	float                 mRaySampleRevPdf;
	unsigned int          mRaySamplingFlags;

	AdditionalRayDataForMis(const UPBPLightVertex *aLightVertices,
							const PathSegment     *aPathSegments,
							MisData               *aCameraVerticesMisData,
							const unsigned int    aCameraPathLength,
							const unsigned int    aMinPathLength,
							const unsigned int    aMaxPathLength,
							const BeamType        aQueryBeamType,
							const BeamType        aPhotonBeamType,
							const float           aLastPdfWInv,
							const float           aSurfMisWeightFactor,
							const float           aPP3DMisWeightFactor,
							const float           aPB2DMisWeightFactor,
							const float           aBB1DMisWeightFactor,
							PhotonBeamsEvaluator  *aBB1DPhotonBeams,
							const float           aBB1DMinMFP,
							const float           aLightSubPathCount,
							const float           aMinDistToMed,
							const float           aRaySamplePdf = 0,
							const float           aRaySampleRevPdf = 0,
							const unsigned int    aRaySamplingFlags = 0) :
	mLightVertices(aLightVertices),
	mPathSegments(aPathSegments),
	mCameraVerticesMisData(aCameraVerticesMisData),
	mCameraPathLength(aCameraPathLength),
	mMinPathLength(aMinPathLength),
	mMaxPathLength(aMaxPathLength),
	mQueryBeamType(aQueryBeamType),
	mPhotonBeamType(aPhotonBeamType),
	mLastPdfWInv(aLastPdfWInv),
	mSurfMisWeightFactor(aSurfMisWeightFactor),
	mPP3DMisWeightFactor(aPP3DMisWeightFactor),
	mPB2DMisWeightFactor(aPB2DMisWeightFactor),
	mBB1DMisWeightFactor(aBB1DMisWeightFactor),
	mBB1DPhotonBeams(aBB1DPhotonBeams),
	mBB1DMinMFP(aBB1DMinMFP),
	mLightSubPathCount(aLightSubPathCount),
	mMinDistToMed(aMinDistToMed),
	mRaySamplePdf(aRaySamplePdf),
	mRaySampleRevPdf(aRaySampleRevPdf),
	mRaySamplingFlags(aRaySamplingFlags)
	{}
};

/**
 * @brief	Grid statistics.
 *
 * 			Number of beams, cells and overfull cells intersected by a single primary ray.
 */
struct GridStats
{
	uint32_t intersectedBeams;  //!< The number of intersected beams.
	uint32_t intersectedCells;  //!< The number of all intersected cells.
	uint32_t overfullCells;     //!< The number of overfull intersected cells.

	/**
	 * @brief	Default constructor.
	 *
	 * 			Initializes all statistics to zero.
	 */
	GridStats()
	{
		intersectedBeams = 0;
		intersectedCells = 0;
		overfullCells = 0;
	}
};

#endif //__UPBPLIGHTVERTEX_HXX__
