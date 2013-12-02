#pragma once

typedef unsigned char	byte;
typedef unsigned short	uint16;
typedef unsigned int	uint32;

#ifdef _WIN32
typedef unsigned __int64	uint64;
#else
typedef unsigned long long	uint64;
#endif

#ifndef M_PI
#define M_PI			3.14159265358979323846
#endif

#define M_PIf			3.14159265358979323846f
#define M_2PIf			6.28318530717958647693f
#define M_PI_2f			1.57079632679489661923f

#define DELTA_EPSILON	0.0001f

#define DEG2RAD(a)		((a) * (float)(M_PI / 180.0))
#define RAD2DEG(a)		((a) * (float)(180.0 / M_PI))

#define F2B(a)			static_cast<byte>((a) * 255.f)
#define B2F(a)			((a) * (1.f / 255.f))

#define F2W(a)			static_cast<uint16>((a) * 65536.f)
#define W2F(a)			((a) * (1.f / 65536.f))

#define SAFE_DELETE(a)			if (a) { delete (a); (a) = NULL; }
#define SAFE_RELEASE(a)			if (a) { (a)->Release(); (a) = NULL; }

//#if !defined(_countof)
//#define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
//#endif

namespace mr
{

inline float frand() { return rand() * (1.f/RAND_MAX); }

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

#include "vec2.h"
#include "vec3.h"
#include "vec4.h"
#include "color.h"
#include "matrix.h"
#include "bbox.h"
#include "frustum.h"
#include "math3d.h"
