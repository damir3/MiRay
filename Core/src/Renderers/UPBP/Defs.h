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

/**
 * @brief	Disables SSE 4.0/4.1 intrinsics (to be able to run on older CPUs). Uncommenting
 * 			produces a legacy build. Legacy and full-speed builds are otherwise binary-compatible.
 */
//#define LEGACY_CPU

#ifndef likely
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#define   likely(expr) expr
#define unlikely(expr) expr
#else
#define   likely(expr) __builtin_expect(expr,true )
#define unlikely(expr) __builtin_expect(expr,false)
#endif
#endif

//////////////////////////////////////////////////////////////////////////
// Enums

/**
 * @brief	Type of geometry used in a scene.
 */
enum GeometryType
{
	REAL = 0,     //!< Ordinary geometry.
	IMAGINARY = 1 //!< Geometry with no surface used only as containers for media.
};

/**
 * @brief	Type of a beam used either for a query or a stored photon beam.
 */
enum BeamType
{
	SHORT_BEAM = 1, //!< Terminates at scattering in media.
	LONG_BEAM = 2   //!< Continues to a surface hit.
};

/**
 * @brief	How we compute photon radius.
 */
enum RadiusCalculation
{
	CONSTANT_RADIUS = 0,  //!< Constant radius for all photons in iteration.
	KNN_RADIUS = 1        //!< Radius computed according to surrounding photons.
};

/**
 * @brief	Type of reduction of a number of photon beams stored within a grid cell.
 */
enum BeamReduction
{
	PRESAMPLE = 0,      //!< Stored: all beams randomly shuffled, tested: first fixed number of stored beams.
	OFFSET = 1,         //!< Stored: all beams randomly shuffled, tested: fixed number of stored beams beginning at a random offset.
	RESAMPLE_FIXED = 2, //!< Stored: all beams, tested: fixed number of randomly picked stored beams.
	RESAMPLE = 3	    //!< Stored: all beams, tested: all stored beams which were accepted in a random test.
};

/**
 * @brief	Estimators available to combine in the UPBP renderer.
 */
enum EstimatorTechnique
{
	BPT = 4,	//!< Bidirectional path tracer.
	SURF = 8,   //!< Surface photon mapping.
	PP3D = 16,  //!< Medium photon mapping (point vs points).
	PB2D = 32,  //!< BRE (beam vs points).
	BB1D = 64   //!< Photon beams (beam vs beams).
};

/**
 * @brief	Misc flags.
 */
enum OtherSettings
{
	NO_SINE_IN_WEIGHTS = 128, //!< Do not use the sine factor in a MIS weight for the BB1D estimator.
	PREVIOUS = 256,           //!< Run all used estimators in a previous mode.
	COMPATIBLE = 512,         //!< Run all used estimators in a compatible mode.
	BB1D_PREVIOUS = 1024,     //!< Run the BB1D estimator in a previous mode.
};

inline bool isPositive(const float x)
{
	return x > 1e-20f;
}

inline bool isNanInfNeg(const float x)
{
	const uint intVal = *((uint*)&x);
	return ((intVal & 0x7F800000) == 0x7F800000) | (x < 0.f);
}

inline bool isPositive(const vec3 & v)
{
	return isPositive(v.x) && isPositive(v.y) && isPositive(v.z);
}

inline bool isNanInfNeg(const vec3 & v)
{
	return isNanInfNeg(v.x) || isNanInfNeg(v.y) || isNanInfNeg(v.z);
}

