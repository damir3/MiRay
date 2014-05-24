#pragma once

#ifdef _WIN32
typedef unsigned char		byte;
typedef unsigned short		uint16;
typedef unsigned int		uint32;
typedef unsigned __int64	uint64;
#else
typedef uint8_t				byte;
typedef uint16_t			uint16;
typedef uint32_t			uint32;
typedef uint64_t			uint64;
#endif

#ifndef M_PI
#define M_PI			3.14159265358979323846264338327950288
#endif
#define M_PIf			3.14159265358979323846264338327950288f

#define M_2PI			6.28318530717958647692528676655900576
#define M_2PIf			6.28318530717958647692528676655900576f

#ifndef M_PI_2
#define M_PI_2			1.57079632679489661923132169163975144
#endif
#define M_PI_2f			1.57079632679489661923132169163975144f

#define DELTA_EPSILON	0.0001f

#define DEG2RAD(a)		((a) * (float)(M_PI / 180.0))
#define RAD2DEG(a)		((a) * (float)(180.0 / M_PI))

#define F2B(a)			static_cast<byte>((a) * 255.f)
#define B2F(a)			((a) * (1.f / 255.f))

#define F2W(a)			static_cast<uint16>((a) * 65535.f)
#define W2F(a)			((a) * (1.f / 65535.f))

#define SAFE_DELETE(a)	if (a) { delete (a); (a) = NULL; }



#if defined(__SSE__) || defined(_WIN64)
#include <xmmintrin.h>

#define USE_SSE

inline __m128 _mm_abs_ps(const __m128 & val)
{
	static const __m128 SIGN_MASK = _mm_set1_ps(-0.f);
	return _mm_andnot_ps(SIGN_MASK, val);
}
#endif



namespace mr
{

#ifdef _WIN32
inline float frand() { return rand() * (1.f / RAND_MAX); }
#else
#define ARC4RANDOM_MAX	0x100000000
inline float frand() { return arc4random() * (1.f / ARC4RANDOM_MAX); }
#endif

template<class Type>
inline const Type& clamp(const Type& value, const Type& min, const Type& max)
{
	if (value <= min) return min;
	if (value >= max) return max;
	return value;
}

template<typename T>
inline T lerp(const T &a, const T &b, float f)
{
	return T(a + (b - a) * f);
}

template <typename T>
inline T lerp2(T v0, T v1, T v2, T v3, float s, float t)
{
	float vt0 = lerp(float(v0), float(v2), t);
	float vt1 = lerp(float(v1), float(v3), t);
	return T(lerp(vt0, vt1, s));
}

template<class T>
inline T sqr(T value)
{
	return value * value;
}

}

#include <cmath>
#include <cfloat>
#ifdef _WIN32
#define fminf(a, b)		std::min<float>(a, b)
#define fmaxf(a, b)		std::max<float>(a, b)
#endif

#include "vec2.h"
#include "vec3.h"
#include "vec4.h"
#include "color.h"
#include "matrix.h"
#include "bbox.h"
#include "frustum.h"
#include "math3d.h"
