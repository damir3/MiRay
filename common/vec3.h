#pragma once

#include "vec2.h"

namespace mr
{

struct Matrix;

template<typename T>
struct TVec3
{
	T x, y, z;

	TVec3() {}
	TVec3(T f) : x(f), y(f), z(f) {}
	TVec3(T x, T y, T z) : x(x), y(y), z(z) {}
	TVec3(const TVec3<double> & v) : x((T)v.x), y((T)v.y), z((T)v.z) {}
	TVec3(const TVec3<float> & v) : x((T)v.x), y((T)v.y), z((T)v.z) {}
	TVec3(const T v[3]) : x(v[0]), y(v[1]), z(v[2]) {}
	//TVec3(const ColorF & c) : x(c.r), y(c.g), z(c.b) {}

	// casting
	operator T* () { return &x; }
	operator const T* () const { return &x; }
	operator TVec2<T> () const { return TVec2<T>(x, y); }

	//operator TVec3<double> () const {return TVec3<double>(x, y, z);}
	//operator TVec3<float> () const {return TVec3<float>((float)x, (float)y, (float)z);}
	//const TVec3 & operator = (const TVec3<double> & v)
	//{
	//	x = (T)v.x;
	//	y = (T)v.y;
	//	z = (T)v.z;
	//	return *this;
	//}
	//const TVec3 & operator = (const TVec3<float> & v)
	//{
	//	x = v.x;
	//	y = v.y;
	//	z = v.z;
	//	return *this;
	//}

	// assignment operators
	void operator += (const TVec3 & v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
	}
	void operator -= (const TVec3 & v)
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
	}
	void operator *= (const TVec3 & v)
	{
		x *= v.x;
		y *= v.y;
		z *= v.z;
	}
	void operator /= (const TVec3 & v)
	{
		x /= v.x;
		y /= v.y;
		z /= v.z;
	}
	void operator += (T f)
	{
		x += f;
		y += f;
		z += f;
	}
	void operator -= (T f)
	{
		x -= f;
		y -= f;
		z -= f;
	}
	void operator *= (T f)
	{
		x *= f;
		y *= f;
		z *= f;
	}
	void operator /= (T f)
	{
		T fInv = (T)1.0 / f;
		x *= fInv;
		y *= fInv;
		z *= fInv;
	}

	// unary operators
	TVec3 operator + () const
	{
		return *this;
	}
	TVec3 operator - () const
	{
		return TVec3(-x, -y, -z);
	}

	// binary operators
	TVec3 operator + (const TVec3 & v) const
	{
		return TVec3 (x + v.x, y + v.y, z + v.z);
	}
	TVec3 operator - (const TVec3 & v) const
	{
		return TVec3 (x - v.x, y - v.y, z - v.z);
	}
	TVec3 operator * (const TVec3 & v) const
	{
		return TVec3 (x * v.x, y * v.y, z * v.z);
	}
	TVec3 operator / (const TVec3 & v) const
	{
		return TVec3 (x / v.x, y / v.y, z / v.z);
	}

	TVec3 operator + (T f) const
	{
		return TVec3 (x + f, y + f, z + f);
	}
	TVec3 operator - (T f) const
	{
		return TVec3 (x - f, y - f, z - f);
	}
	TVec3 operator * (T f) const
	{
		return TVec3 (x * f, y * f, z * f);
	}
	TVec3 operator / (T f) const
	{
		T fInv = (T)1.0 / f;
		return TVec3(x * fInv, y * fInv, z * fInv);
	}

	friend TVec3 operator + (T f, const TVec3 & v)
	{
		return TVec3(f + v.x, f + v.y, f + v.z);
	}
	friend TVec3 operator - (T f, const TVec3 & v)
	{
		return TVec3(f - v.x, f - v.y, f - v.z);
	}
	friend TVec3 operator * (T f, const TVec3 & v)
	{
		return TVec3(f * v.x, f * v.y, f * v.z);
	}

	bool operator == (const TVec3 & v) const
	{
		return ((v.x == x) && (v.y == y) && (v.z == z));
	}
	bool operator != (const TVec3 & v) const
	{
		return ((v.x != x) || (v.y != y) || (v.z != z));
	}

	// functions

	T Length() const
	{
		return sqrt(x * x + y * y + z * z);
	}
	T LengthSquared() const
	{
		return (x * x + y * y + z * z);
	}

	T Normalize()
	{
		T length = (T)sqrt(x * x + y * y + z * z);
		if (length != (T)0.0) {
			T f = (T)1.0 / length;
			x *= f;
			y *= f;
			z *= f;
		}
		return length;
	}

	T Yaw() const; // in radians
	T Pitch() const; // in radians

	// -------------------------------------------------------------------- //
	
	TVec3 TransformedCoord(const Matrix &mat) const;
	TVec3 TransformedNormal(const Matrix &mat) const;
	void Rotate(const TVec3 &axis, float sin_a, float cos_a);
	void Rotate(const TVec3 &axis, float angle);
	void TransformCoord(const Matrix &mat) { *this = TransformedCoord(mat); }
	void TransformNormal(const Matrix &mat) { *this = TransformedNormal(mat); }
	void TTransformCoord(const Matrix &mat);
	void TTransformNormal(const Matrix &mat);
	TVec3 Perpendicular() const;

	void Scale(const TVec3 &scale)
	{
		x *= scale.x;
		y *= scale.y;
		z *= scale.z;
	}

	// -------------------------------------------------------------------- //

	static const TVec3 Null;
	static const TVec3 X;
	static const TVec3 Y;
	static const TVec3 Z;

	static T Dot(const TVec3 & a, const TVec3 & b)
	{
		return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
	}
	static TVec3 Cross(const TVec3 & a, const TVec3 & b)
	{
		return TVec3((a.y * b.z) - (a.z * b.y),
			(a.z * b.x) - (a.x * b.z),
			(a.x * b.y) - (a.y * b.x));
	}
	static TVec3 Lerp(const TVec3 & a, const TVec3 & b, T f)
	{
		return TVec3(a.x + (b.x - a.x) * f,
			a.y + (b.y - a.y) * f,
			a.z + (b.z - a.z) * f);
	}
	static TVec3 Lerp3(const TVec3 & a, const TVec3 & b, const TVec3 & f)
	{
		return TVec3(a.x + (b.x - a.x) * f.x,
					 a.y + (b.y - a.y) * f.y,
					 a.z + (b.z - a.z) * f.z);
	}
	static TVec3 Normalize(const TVec3 & v)
	{
		T lengthSquared = v.x * v.x + v.y * v.y + v.z * v.z;
		if (lengthSquared != (T)0.0) {
			T f = (T)1.0 / (T)sqrt(lengthSquared);
			return TVec3(v.x * f, v.y * f, v.z * f);
		}
		return TVec3::Null;
	}

	static TVec3 Reflect(const TVec3 & i, const TVec3 & n)
	{
		return i - n * ((T)2.0 * Dot(i, n));
	}

	static TVec3 Refract(const TVec3 & i, const TVec3 & n, float eta)
	{
		T IdotN = TVec3::Dot(i, n);
		T cost2 = (T)1.0 - eta * eta * ((T)1.0 - IdotN * IdotN);
		return cost2 > (T)0.0 ? (i * eta - n * (eta * IdotN + (T)sqrt(cost2))) : TVec3::Null;
	}
};

typedef TVec3<float>	Vec3;
typedef TVec3<float>	Vec3F;
typedef TVec3<double>	Vec3D;

}