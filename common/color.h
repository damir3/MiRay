#pragma once

namespace mr
{

struct ColorF;

struct Color
{
	byte r, g, b, a;

	Color() {}
	Color(byte bRed, byte bGreen, byte bBlue, byte bAlpha = 255) : r(bRed), g(bGreen), b(bBlue), a(bAlpha) {}
	//Color(const Vec4 & v) : r(F2B(v.x)), g(F2B(v.y)), b(F2B(v.z)), a(F2B(v.w)) {}
	//Color(const Vec3 & v, float fAlpha = 1.f) : r(F2B(v.x)), g(F2B(v.y)), b(F2B(v.z)), a(F2B(fAlpha)) {}
	Color(const byte c[4]) : r(c[0]), g(c[1]), b(c[2]), a(c[3]) {}
	Color(uint32 c) : r((byte)c), g((byte)(c>>8)), b((byte)(c>>16)), a((byte)(c>>24)) {}

	operator uint32 () const
	{
		return (a << 24) + (b << 16) + (g << 8) + (r);
	}
	operator ColorF () const;
	operator const byte* () const { return (byte *)this; }

	bool operator == (const Color &c) const {return r == c.r && g == c.g && b == c.b && a == c.a;}
	
	static Color Lerp(const Color &from, const Color &to, float f)
	{
		if (from == to) return from;

		float r = from.r + (to.r - from.r) * f;
		float g = from.g + (to.g - from.g) * f;
		float b = from.b + (to.b - from.b) * f;
		float a = from.a + (to.a - from.a) * f;
		return Color((byte)r, (byte)g, (byte)b, (byte)a);
	}

	static const Color Null;
	static const Color White;
};

struct ColorF
{
	float r, g, b, a;

	ColorF() {}
	ColorF(float bRed, float bGreen, float bBlue, float bAlpha = 1.f) : r(bRed), g(bGreen), b(bBlue), a(bAlpha) {}
	ColorF(const Vec4 & v) : r(v.x), g(v.y), b(v.z), a(v.w) {}
	ColorF(const Vec3 & v, float fAlpha = 1.f) : r(v.x), g(v.y), b(v.z), a(fAlpha) {}
	ColorF(const float c[4]) : r(c[0]), g(c[1]), b(c[2]), a(c[3]) {}

	operator Color () const
	{
		Color c;
		c.r = F2B(clamp(r, 0.f, 1.f));
		c.g = F2B(clamp(g, 0.f, 1.f));
		c.b = F2B(clamp(b, 0.f, 1.f));
		c.a = F2B(clamp(a, 0.f, 1.f));
		return c;
	}

	operator const float* () const { return (float *)this; }

	bool operator == (const ColorF &ref) const 
	{
		if (fabs(r - ref.r) > DELTA_EPSILON) return false;
		if (fabs(g - ref.g) > DELTA_EPSILON) return false;
		if (fabs(b - ref.b) > DELTA_EPSILON) return false;
		if (fabs(a - ref.a) > DELTA_EPSILON) return false;
		return true;
	}

	bool operator != (const ColorF &ref) const 
	{
		if (fabs(r - ref.r) > DELTA_EPSILON) return true;
		if (fabs(g - ref.g) > DELTA_EPSILON) return true;
		if (fabs(b - ref.b) > DELTA_EPSILON) return true;
		if (fabs(a - ref.a) > DELTA_EPSILON) return true;
		return false;
	}

	void operator += (const ColorF &c)
	{
		r += c.r; g += c.g; b += c.b; a += c.a;
	}
	void operator -= (const ColorF &c)
	{
		r -= c.r; g -= c.g; b -= c.b; a -= c.a;
	}
	void operator *= (float scale)
	{
		r *= scale;	g *= scale; b *= scale; a *= scale;
	}
	void operator *= (ColorF c)
	{
		r *= c.r; g *= c.g; b *= c.b; a *= c.a;
	}
	void operator /= (float k)
	{
		float	scale = 1.f / k;
		r *= scale;	g *= scale; b *= scale; a *= scale;
	}

	ColorF operator + (const ColorF &c) const
	{
		return ColorF(r + c.r, g + c.g, b + c.b, a + c.a);
	}
	ColorF operator - (const ColorF &c) const
	{
		return ColorF(r - c.r, g - c.g, b - c.b, a - c.a);
	}
	ColorF operator * (float scale) const
	{
		return ColorF(r * scale, g * scale, b * scale, a * scale);
	}
	ColorF operator * (ColorF c) const
	{
		return ColorF(r * c.r, g * c.g, b * c.b, a * c.a);
	}
	ColorF operator / (float k) const
	{
		float	scale = 1.f / k;
		return ColorF(r * scale, g * scale, b * scale, a * scale);
	}

	void Saturate()
	{
		r = clamp(r, 0.f, 1.f);
		g = clamp(g, 0.f, 1.f);
		b = clamp(b, 0.f, 1.f);
		a = clamp(a, 0.f, 1.f);
	}

	float ToGrayscale() const
	{
		return 0.2989f * r + 0.5870f * g + 0.1140f * b;
	}

	static ColorF Lerp(const ColorF &from, const ColorF &to, float f)
	{
		if (from == to) return from;

		float r = from.r + (to.r - from.r) * f;
		float g = from.g + (to.g - from.g) * f;
		float b = from.b + (to.b - from.b) * f;
		float a = from.a + (to.a - from.a) * f;
		return ColorF(r, g, b, a);
	}

	static ColorF LerpRGB(const ColorF &from, const ColorF &to, float f, float a)
	{
		if (from == to) return ColorF(from.r, from.g, from.b, a);

		float r = from.r + (to.r - from.r) * f;
		float g = from.g + (to.g - from.g) * f;
		float b = from.b + (to.b - from.b) * f;
		return ColorF(r, g, b, a);
	}

	static const ColorF Null;
	static const ColorF White;
};

inline Color::operator ColorF () const
{
	ColorF c;
	c.r = B2F(r);
	c.g = B2F(g);
	c.b = B2F(b);
	c.a = B2F(a);
	return c;
}

}