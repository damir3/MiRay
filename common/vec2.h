#pragma once

namespace mr
{

template<typename T>
struct TVec2
{
	T x, y;

	TVec2() {}
	TVec2(T f) : x(f), y(f) {}
	TVec2(T fx, T fy) : x(fx), y(fy) {}
	TVec2(const TVec2<float> & v) : x((T)v.x), y((T)v.y) {}
	TVec2(const TVec2<double> & v) : x((T)v.x), y((T)v.y) {}
	TVec2(const T v[2]) : x(v[0]), y(v[1]) {}

	// casting
	operator T* () { return &x; }
	operator const T* () const { return &x; }

	//const TVec2 & operator = (const TVec2<double> & v)
	//{
	//	x = (T)v.x;
	//	y = (T)v.y;
	//	return *this;
	//}
	//const TVec2 & operator = (const TVec2<float> & v)
	//{
	//	x = v.x;
	//	y = v.y;
	//	return *this;
	//}

	// assignment operators
	void operator += (const TVec2 & v)
	{
		x += v.x;
		y += v.y;
	}
	void operator -= (const TVec2 & v)
	{
		x -= v.x;
		y -= v.y;
	}
	void operator *= (const TVec2 & v)
	{
		x *= v.x;
		y *= v.y;
	}
	void operator /= (const TVec2 & v)
	{
		x /= v.x;
		y /= v.y;
	}

	void operator += (T f)
	{
		x += f;
		y += f;
	}
	void operator -= (T f)
	{
		x -= f;
		y -= f;
	}
	void operator *= (T f)
	{
		x *= f;
		y *= f;
	}
	void operator /= (T f)
	{
		T	fInv = 1.0 / f;
		x *= fInv;
		y *= fInv;
	}

	// unary operators
	TVec2 operator + () const
	{
		return *this;
	}
	TVec2 operator - () const
	{
		return TVec2 (-x, -y);
	}

	// binary operators
	TVec2 operator + (const TVec2 & v) const
	{
		return TVec2 (x + v.x, y + v.y);
	}
	TVec2 operator - (const TVec2 & v) const
	{
		return TVec2 (x - v.x, y - v.y);
	}
	TVec2 operator * (const TVec2 & v) const
	{
		return TVec2 (x * v.x, y * v.y);
	}
	TVec2 operator / (const TVec2 & v) const
	{
		return TVec2 (x / v.x, y / v.y);
	}

	TVec2 operator + (T f) const
	{
		return TVec2 (x + f, y + f);
	}
	TVec2 operator - (T f) const
	{
		return TVec2 (x - f, y - f);
	}
	TVec2 operator * (T f) const
	{
		return TVec2 (x * f, y * f);
	}
	TVec2 operator / (T f) const
	{
		T fInv = (T)1.0 / f;
		return TVec2 (x * fInv, y * fInv);
	}

	friend TVec2 operator + (T f, const TVec2 & v)
	{
		return TVec2 (f + v.x, f + v.y);
	}
	friend TVec2 operator - (T f, const TVec2 & v)
	{
		return TVec2 (f - v.x, f - v.y);
	}
	friend TVec2 operator * (T f, const TVec2 & v)
	{
		return TVec2 (f * v.x, f * v.y);
	}

	bool operator == (const TVec2 &v) const
	{
		return ((v.x == x) && (v.y == y));
	}
	bool operator != (const TVec2 &v) const
	{
		return ((v.x != x) || (v.y != y));
	}

	// functions

	T Length() const
	{
		return (T)sqrt(x * x + y * y);
	}
	T LengthSquared() const
	{
		return (x * x + y * y);
	}

	T Normalize()
	{
		T length = (T)sqrt(x * x + y * y);
		if (length != (T)0.0) {
			T f = (T)1.0 / length;
			x *= f;
			y *= f;
		}
		return length;
	}

	// -------------------------------------------------------------------- //

	static const TVec2 Null, X, Y;

	static T Dot(const TVec2& a, const TVec2& b)
	{
		return (a.x * b.x) + (a.y * b.y);
	}
	static TVec2 Cross(const TVec2& a)
	{
		return TVec2(a.y, -a.x);
	}
	static TVec2 Lerp(const TVec2& a, const TVec2& b, T f)
	{
		return TVec2(a.x + (b.x - a.x) * f, a.y + (b.y - a.y) * f);
	}
	static TVec2 Normalize(const TVec2 &v)
	{
		T lengthSquared = v.x * v.x + v.y * v.y;
		if (lengthSquared != (T)0.0) {
				T f = (T)1.0 / (T)sqrt(lengthSquared);
			return TVec2(v.x * f, v.y * f);
		}
		return TVec2::Null;
	}

	static TVec2 Reflect(const TVec2 &v, const TVec2 &n)
	{
		return v - 2 * Dot(v, n) * n;
	}

	static TVec2 Refract(const TVec2 &v, const TVec2 &n, float fRatio)
	{
		// based on http://http.developer.nvidia.com/Cg/refract.html
		float cosi = TVec2::Dot(-v, n);
		float cost2 = 1.0f - fRatio * fRatio * (1.0f - cosi*cosi);
		if (cost2 <= 0) return TVec2::Null;
		return fRatio * v + (fRatio * cosi - sqrtf(fabs(cost2))) * n;
	}
};

typedef TVec2<float>	Vec2;
typedef TVec2<float>	Vec2F;
typedef TVec2<double>	Vec2D;

}