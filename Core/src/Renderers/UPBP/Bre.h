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

#ifndef __BRE_HXX__
#define __BRE_HXX__

#include "UPBPLightVertex.h"

class EmbreePhoton;

/**
 * @brief	Support for embree-based beam radiance estimate.
 */
class EmbreeBre
{
public:

	/**
	 * @brief	Constructor.
	 *
	 * @param	aScene	The scene.
	 */
	EmbreeBre(const Scene& aScene)
		: scene(aScene)
		, embreePhotons(nullptr)
		, numEmbreePhotons(0)
		, m_rtcScene(nullptr)
	{
	}

	/**
	 * @brief	Empty destructor.
	 */
	~EmbreeBre();

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
	int Build(RTCDevice device,
			  const UPBPLightVertex* lightSubPathVertices,
			  const int numVertices,
			  RadiusCalculation radiusCalculation,
			  const float photonRadius,
			  const int knn);

	/**
	 * @brief	Destroy the data structure for BRE queries.
	 */
	void Destroy();

	/**
	 * @brief	Evaluates the beam radiance estimate for the given query ray.
	 *
	 * @param	beamType					   	Type of the beam.
	 * @param	queryRay					   	The query ray (=beam) for the beam radiance estimate.
	 * @param	segments					   	Full volume segments of media intersected by the ray.
	 * @param	estimatorTechniques			   	(Optional) the estimator techniques to use.
	 * @param	raySamplingFlags			   	(Optional) the ray sampling flags (\c kOriginInMedium).
	 * @param [in,out]	additionalRayDataForMis	(Optional) additional data needed for MIS weights
	 * 											computations.
	 *
	 * @return	The accumulated radiance along the ray.
	 */
	vec3 EvalBre(BeamType beamType,
				 const Ray& queryRay,
				 const VolumeSegments& segments,
				 const uint estimatorTechniques = PB2D,
				 const uint raySamplingFlags = 0,
				 AdditionalRayDataForMis* additionalRayDataForMis = nullptr);

protected:

	const Scene &	scene;				//!< Reference to the scene for material evaluation.
	EmbreePhoton *	embreePhotons;		//!< Our structure into which we've converted the input path vertices.
	int				numEmbreePhotons;	//!< Number of elements in the embreePhotons array.
	RTCScene		m_rtcScene;			//!< Embree's data structure storing the photon spheres.
};

#endif // __BRE_HXX__
