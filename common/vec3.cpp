#include "vec3.h"
#include "matrix.h"

using namespace mr;

// ----------------------------------------------------------------------------
// static const variables
// ----------------------------------------------------------------------------

template<> const Vec3F Vec3F::Null(0.f, 0.f, 0.f);
template<> const Vec3F Vec3F::X(1.f, 0.f, 0.f);
template<> const Vec3F Vec3F::Y(0.f, 1.f, 0.f);
template<> const Vec3F Vec3F::Z(0.f, 0.f, 1.f);

template<> const Vec3D Vec3D::Null(0.0, 0.0, 0.0);
template<> const Vec3D Vec3D::X(1.0, 0.0, 0.0);
template<> const Vec3D Vec3D::Y(0.0, 1.0, 0.0);
template<> const Vec3D Vec3D::Z(0.0, 0.0, 1.0);

// ----------------------------------------------------------------------------
// static functions
// ----------------------------------------------------------------------------

//float	Vec3::Dot(const Vec3 &a, const Vec3 &b)
//{
//	return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
//}
//
//Vec3 Vec3::Cross(const Vec3 &a, const Vec3 &b)
//{
//	return Vec3((a.y * b.z) - (a.z * b.y),
//				(a.z * b.x) - (a.x * b.z),
//				(a.x * b.y) - (a.y * b.x));
//}

// ----------------------------------------------------------------------------
// casting
// ----------------------------------------------------------------------------

//Vec3::operator float* ()
//{
//	return (float *) &x;
//}
//
//Vec3::operator const float* () const
//{
//	return (const float *) &x;
//}

// ----------------------------------------------------------------------------
// assignment operators
// ----------------------------------------------------------------------------

//void Vec3::operator += (const Vec3 &v) {
//	x += v.x;
//	y += v.y;
//	z += v.z;
//}
//
//void Vec3::operator -= (const Vec3 &v) {
//	x -= v.x;
//	y -= v.y;
//	z -= v.z;
//}
//
//void Vec3::operator *= (const Vec3 &v) {
//	x *= v.x;
//	y *= v.y;
//	z *= v.z;
//}
//
//void Vec3::operator /= (const Vec3 &v) {
//	x /= v.x;
//	y /= v.y;
//	z /= v.z;
//}
//
//void Vec3::operator += (float f) {
//	x += f;
//	y += f;
//	z += f;
//}
//
//void Vec3::operator -= (float f) {
//	x -= f;
//	y -= f;
//	z -= f;
//}
//
//void Vec3::operator *= (float f) {
//	x *= f;
//	y *= f;
//	z *= f;
//}
//
//void Vec3::operator /= (float f) {
//	float fInv = 1.0f / f;
//	x *= fInv;
//	y *= fInv;
//	z *= fInv;
//}

// ----------------------------------------------------------------------------
// unary operators
// ----------------------------------------------------------------------------

//Vec3 Vec3::operator + () const {
//	return *this;
//}
//
//Vec3 Vec3::operator - () const {
//	return Vec3(-x, -y, -z);
//}

// ----------------------------------------------------------------------------
// binary operators
// ----------------------------------------------------------------------------

//Vec3 Vec3::operator + (const Vec3 &other) const {
//	return Vec3 (x + other.x, y + other.y, z + other.z);
//}
//
//Vec3 Vec3::operator - (const Vec3 &other) const {
//	return Vec3 (x - other.x, y - other.y, z - other.z);
//}
//
//Vec3 Vec3::operator * (const Vec3 &other) const {
//	return Vec3 (x * other.x, y * other.y, z * other.z);
//}
//
//Vec3 Vec3::operator / (const Vec3 &other) const {
//	return Vec3 (x / other.x, y / other.y, z / other.z);
//}
//
//Vec3 Vec3::operator + (float f) const {
//	return Vec3 (x + f, y + f, z + f);
//}
//
//Vec3 Vec3::operator - (float f) const {
//	return Vec3 (x - f, y - f, z - f);
//}
//
//Vec3 Vec3::operator * (float f) const {
//	return Vec3 (x * f, y * f, z * f);
//}
//
//Vec3 Vec3::operator / (float f) const {
//	float fInv = 1.0F / f;
//	return Vec3(x * fInv, y * fInv, z * fInv);
//}

// ----------------------------------------------------------------------------

//Vec3 operator + (float f, const Vec3 &v) {
//	return Vec3(f + v.x, f + v.y, f + v.z);
//}
//
//Vec3 operator - (float f, const Vec3 &v) {
//	return Vec3(f - v.x, f - v.y, f - v.z);
//}
//
//Vec3 operator * (float f, const Vec3 &v) {
//	return Vec3(f * v.x, f * v.y, f * v.z);
//}

// ----------------------------------------------------------------------------

//bool	Vec3::operator == (const Vec3 &v) const {
//	//return ((v.x == x) && (v.y == y) && (v.z == z));
//
//	const float f = FLOAT_EQUALITY_FUDGE;
//	if (v.x-x < -f) return false;
//	if (v.x-x > f) return false;
//	if (v.y-y < -f) return false;
//	if (v.y-y > f) return false;
//	if (v.z-z < -f) return false;
//	if (v.z-z > f) return false;
//	return true;
//}

//bool	Vec3::operator != (const Vec3 &v) const {
//	//return ((v.x != x) || (v.y != y) || (v.z == z));
//
//	const float f = FLOAT_EQUALITY_FUDGE;
//	if (v.x-x < -f) return true;
//	if (v.x-x > f) return true;
//	if (v.y-y < -f) return true;
//	if (v.y-y > f) return true;
//	if (v.z-z < -f) return true;
//	if (v.z-z > f) return true;
//	return false;
//}

// ----------------------------------------------------------------------------
// functions
// ----------------------------------------------------------------------------

//float	Vec3::Length() const {
//	return sqrtf(x * x + y * y + z * z);
//}
//
//float	Vec3::Length2() const {
//	return (x * x + y * y + z * z);
//}
//
//float	Vec3::Normalize() {
//	float length = sqrtf(x * x + y * y + z * z);
//	if (length != 0) {
//		float	k = 1.0F / length;
//		x *= k;
//		y *= k;
//		z *= k;
//	}
//	return length;
//}

template<> 
float Vec3::Yaw() const
{
	return atan2(y, x);
}

template<> 
float Vec3::Pitch() const
{
	return atan2(z, sqrtf(x * x + y * y));
}


template<> 
void Vec3::Rotate(const Vec3 &axis, float sin_a, float cos_a)
{
	float	one_c;
	float	xx, yy, zz, xy, yz, zx, xs, ys, zs;
	float	src_x, src_y, src_z;

	one_c = 1.0F - cos_a;
	xx = axis.x * axis.x;
	yy = axis.y * axis.y;
	zz = axis.z * axis.z;
	xy = axis.x * axis.y;
	yz = axis.y * axis.z;
	zx = axis.z * axis.x;
	xs = axis.x * sin_a;
	ys = axis.y * sin_a;
	zs = axis.z * sin_a;

	src_x = x; src_y = y; src_z = z;
	x = src_x*((one_c * xx) + cos_a)  +
		src_y*((one_c * xy) + zs) +
		src_z*((one_c * zx) - ys);
	y = src_x*((one_c * xy) - zs) +
		src_y*((one_c * yy) + cos_a)  +
		src_z*((one_c * yz) + xs);
	z = src_x*((one_c * zx) + ys) +
		src_y*((one_c * yz) - xs) +
		src_z*((one_c * zz) + cos_a);
}

template<> 
void Vec3::Rotate(const Vec3 &axis, float rad)
{
	float	one_c, sin_a, cos_a;
	float	xx, yy, zz, xy, yz, zx, xs, ys, zs;
	float	src_x, src_y, src_z;
	
	sin_a = sinf(rad);
	cos_a = cosf(rad);
	one_c = 1.0F - cos_a;
	xx = axis.x * axis.x;
	yy = axis.y * axis.y;
	zz = axis.z * axis.z;
	xy = axis.x * axis.y;
	yz = axis.y * axis.z;
	zx = axis.z * axis.x;
	xs = axis.x * sin_a;
	ys = axis.y * sin_a;
	zs = axis.z * sin_a;

	src_x = x; src_y = y; src_z = z;
	x = src_x*((one_c * xx) + cos_a)  +
		src_y*((one_c * xy) + zs) +
		src_z*((one_c * zx) - ys);
	y = src_x*((one_c * xy) - zs) +
		src_y*((one_c * yy) + cos_a)  +
		src_z*((one_c * yz) + xs);
	z = src_x*((one_c * zx) + ys) +
		src_y*((one_c * yz) - xs) +
		src_z*((one_c * zz) + cos_a);
}

template<> 
void Vec3::TransformNormal(const Matrix &mat)
{
	Vec3 tmp = *(this);
	x = tmp.x*mat.m11 + tmp.y*mat.m21 + tmp.z*mat.m31;
	y = tmp.x*mat.m12 + tmp.y*mat.m22 + tmp.z*mat.m32;
	z = tmp.x*mat.m13 + tmp.y*mat.m23 + tmp.z*mat.m33;
}

template<> 
void Vec3::TransformCoord(const Matrix &mat)
{
	Vec3 tmp = *(this);
	x = tmp.x*mat.m11 + tmp.y*mat.m21 + tmp.z*mat.m31 + mat.m41;
	y = tmp.x*mat.m12 + tmp.y*mat.m22 + tmp.z*mat.m32 + mat.m42;
	z = tmp.x*mat.m13 + tmp.y*mat.m23 + tmp.z*mat.m33 + mat.m43;
}

template<> 
void Vec3::TTransformNormal(const Matrix &mat)
{
	Vec3 tmp = *(this);
	x = tmp.x*mat.m11 + tmp.y*mat.m12 + tmp.z*mat.m13;
	y = tmp.x*mat.m21 + tmp.y*mat.m22 + tmp.z*mat.m23;
	z = tmp.x*mat.m31 + tmp.y*mat.m32 + tmp.z*mat.m33;
}

template<> 
void Vec3::TTransformCoord(const Matrix &mat)
{
	Vec3 tmp = *(this);
	x = tmp.x*mat.m11 + tmp.y*mat.m12 + tmp.z*mat.m13 + mat.m14;
	y = tmp.x*mat.m21 + tmp.y*mat.m22 + tmp.z*mat.m23 + mat.m24;
	z = tmp.x*mat.m31 + tmp.y*mat.m32 + tmp.z*mat.m33 + mat.m34;
}

template<> 
void Vec3::Perpendicular()
{
	//if (x == 0 && y == 0 && z == 0) return;

	if (fabsf(x) <= fabsf(y) && fabsf(x) <= fabsf(z))
		*this = Vec3(y*y+z*z, -x*y, -x*z);
	else if (fabsf(y) <= fabsf(z))
		*this = Vec3(-y*x, x*x+z*z, -y*z);
	else 
		*this = Vec3(-z*x, -z*y, x*x+y*y);

	Normalize();
}

