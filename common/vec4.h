#pragma once

#include "vec3.h"

namespace mr
{

struct Matrix;

template<typename T>
struct TVec4
{
	T x, y, z, w;

	TVec4() {}
	TVec4(T fx, T fy, T fz, T fw) : x(fx), y(fy), z(fz), w(fw) {}
	TVec4(const TVec4<double> & v) : x((T)v.x), y((T)v.y), z((T)v.z), w((T)v.w) {}
	TVec4(const TVec4<float> & v) : x(v.x), y(v.y), z(v.z), w(v.w) {}
	TVec4(const T v[4]) : x(v[0]), y(v[1]), z(v[2]), w(v[3]) {}
	//TVec4(const ColorF & c) : x(c.r), y(c.g), z(c.b), w(c.a) {}
	TVec4(const TVec3<T> &v, T fw) : x(v.x), y(v.y), z(v.z), w(fw) {}

	// casting
	operator T* () { return &x; }
	operator const T* () const { return &x; }
	operator TVec3<T> () const { return TVec3<T>(x, y, z); }

	void Transform(const Matrix &mat);
	void TTransform(const Matrix &mat);

	// -------------------------------------------------------------------- //

	static const TVec4 Null;

	static T Dot(const TVec4 &a, const TVec4 &b)
	{
		return (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
	}
	static TVec4 Cross(const TVec4 &a, const TVec4 &b, const TVec4 &c)
	{
		TVec4 result;
		result.x =  TVec3<T>::Dot(TVec3<T>(a.y, b.y, c.y), TVec3<T>::Cross(TVec3<T>(a.z, b.z, c.z), TVec3<T>(a.w, b.w, c.w)));
		result.y = -TVec3<T>::Dot(TVec3<T>(a.x, b.x, c.x), TVec3<T>::Cross(TVec3<T>(a.z, b.z, c.z), TVec3<T>(a.w, b.w, c.w)));
		result.z =  TVec3<T>::Dot(TVec3<T>(a.x, b.x, c.x), TVec3<T>::Cross(TVec3<T>(a.y, b.y, c.y), TVec3<T>(a.w, b.w, c.w)));
		result.w = -TVec3<T>::Dot(TVec3<T>(a.x, b.x, c.x), TVec3<T>::Cross(TVec3<T>(a.y, b.y, c.y), TVec3<T>(a.z, b.z, c.z)));
		return result;
	}
	static TVec4 Lerp(const TVec4 &a, const TVec4 &b, T f)
	{
		return TVec4(a.x + (b.x - a.x) * f,
			a.y + (b.y - a.y) * f,
			a.z + (b.z - a.z) * f,
			a.w + (b.w - a.w) * f);
	}
};

typedef TVec4<float>	Vec4;
typedef TVec4<float>	Vec4F;
typedef TVec4<double>	Vec4D;

}