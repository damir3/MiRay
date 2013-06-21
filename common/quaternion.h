#pragma once

namespace mr
{

struct Quaternion;
void QuaternionMultiply(Quaternion *out, const Quaternion *a, const Quaternion *b);

struct Quaternion
{
	float x, y, z, w;

	Quaternion() {}
	Quaternion(const float* v) : x(v[0]), y(v[1]), z(v[2]), w(v[3]) {}
	Quaternion(float fx, float fy, float fz, float fw) : x(fx), y(fy), z(fz), w(fw) {}

	// casting
	operator float* () {
		return (float *) &x;
	}
	operator const float* () const {
		return (const float *) &x;
	}

	// assignment operators
	Quaternion& operator += (const Quaternion & q)
	{
		x += q.x;
		y += q.y;
		z += q.z;
		w += q.w;
		return *this;
	}
	Quaternion& operator -= (const Quaternion & q)
	{
		x -= q.x;
		y -= q.y;
		z -= q.z;
		w -= q.w;
		return *this;
	}
	Quaternion& operator *= (const Quaternion & q)
	{
		QuaternionMultiply(this, &q, this);
		return *this;
	}
	Quaternion& operator *= (float f)
	{
		x *= f;
		y *= f;
		z *= f;
		w *= f;
		return *this;
	}
	Quaternion& operator /= (float f)
	{
		float fInv = 1.f / f;
		x *= fInv;
		y *= fInv;
		z *= fInv;
		w *= fInv;
		return *this;
	}

	// unary operators
	Quaternion operator + () const
	{
		return *this;
	}
	Quaternion operator - () const
	{
		return Quaternion(-x, -y, -z, -w);
	}

	// binary operators
	Quaternion operator + (const Quaternion & q) const
	{
		return Quaternion(x + q.x, y + q.y, z + q.z, w + q.w);
	}
	Quaternion operator - (const Quaternion & q) const
	{
		return Quaternion(x - q.x, y - q.y, z - q.z, w - q.w);
	}
	Quaternion operator * (const Quaternion & q) const
	{
		Quaternion qT;
		QuaternionMultiply(&qT, this, &q);
		return qT;
	}
	Quaternion operator * (float f) const
	{
		return Quaternion(x * f, y * f, z * f, w * f);
	}
	Quaternion operator / (float f) const
	{
		float fInv = 1.f / f;
		return Quaternion(x * fInv, y * fInv, z * fInv, w * fInv);
	}

	friend Quaternion operator * (float f, const Quaternion & q)
	{
		return Quaternion(f * q.x, f * q.y, f * q.z, f * q.w);
	}

	bool operator == (const Quaternion & q) const
	{
		return x == q.x && y == q.y && z == q.z && w == q.w;
	}
	bool operator != (const Quaternion & q) const
	{
		return x != q.x || y != q.y || z != q.z || w != q.w;
	}

	// functions
	float Length()
	{
		return sqrtf(x*x + y*y + z*z + w*w);
	}
	float Length2()
	{
		return x*x + y*y + z*z + w*w;
	}
	float Normalize()
	{
		float length = Length();
		if (length != 0) {
			float f = 1.f / length;
			x *= f;
			y *= f;
			z *= f;
			w *= f;
		}
		return length;
	}
	void SetIdentity()
	{
		x = y = z = 0.f;
		w = 1.f;
	}
	void Inverse()
	{
		*this /= -Length2();
	}
	void Conjugate()
	{
		*this = Quaternion(-x, -y, -z, -w);
	}
	void Slerp(const Quaternion &a, const Quaternion &b, float t);
	void GetAxisAngle(Vec3 &axis, float &angle_rad);
	void RotationAxis(const Vec3 &axis, float angle_rad);
	void RotationMatrix(const Matrix &);
	void RotationYawPitchRoll(float yaw, float pitch, float roll);

	// -------------------------------------------------------------------- //

	static const Quaternion Identity;

	static float Dot(const Quaternion &a, const Quaternion &b);
};

inline void QuaternionMultiply(Quaternion *out, const Quaternion *a, const Quaternion *b)
{
	Quaternion qT;
	qT.x = a->x * b->w + b->x * a->w + a->y * b->z - a->z * b->y;
	qT.y = a->y * b->w + b->y * a->w + a->z * b->x - a->x * b->z;
	qT.z = a->z * b->w + b->z * a->w + a->x * b->y - a->y * b->x;
	qT.w = a->w * b->w - a->x * b->x - a->y * b->y - a->z * b->z;
	*out = qT;
}

}