#include "vec2.h"

using namespace mr;

// ----------------------------------------------------------------------------
// static const variables
// ----------------------------------------------------------------------------

template<> const Vec2F Vec2F::Null(0.f, 0.f);
template<> const Vec2F Vec2F::X(1.f, 0.f);
template<> const Vec2F Vec2F::Y(0.f, 1.f);

template<> const Vec2D Vec2D::Null(0.0, 0.0);
template<> const Vec2D Vec2D::X(1.0, 0.0);
template<> const Vec2D Vec2D::Y(0.0, 1.0);

// ----------------------------------------------------------------------------
// static functions
// ----------------------------------------------------------------------------

//float Vec2::Dot(const Vec2 &a, const Vec2 &b) {
//	return (a.x * b.x) + (a.y * b.y);
//}

/*Vec2 Vec2::Cross(const Vec2 &a, const Vec2 &b) {
	Vec2 d;
	d.x = (a.y * b.z) - (a.z * b.y);
	d.y = (a.z * b.x) - (a.x * b.z);
	d.z = (a.x * b.y) - (a.y * b.x);
	return d;
}*/

// ----------------------------------------------------------------------------
// casting
// ----------------------------------------------------------------------------

//Vec2::operator float* () {
//	return (float *) &x;
//}
//
//Vec2::operator const float* () const {
//	return (const float *) &x;
//}

// ----------------------------------------------------------------------------
// assignment operators
// ----------------------------------------------------------------------------

//void	Vec2::operator += (const Vec2 &v) {
//	x += v.x;
//	y += v.y;
//}
//
//void	Vec2::operator -= (const Vec2 &v) {
//	x -= v.x;
//	y -= v.y;
//}
//
//void	Vec2::operator *= (const Vec2 &v) {
//	x *= v.x;
//	y *= v.y;
//}
//
//void	Vec2::operator /= (const Vec2 &v) {
//	x /= v.x;
//	y /= v.y;
//}
//
//void	Vec2::operator += (float f) {
//	x += f;
//	y += f;
//}
//
//void	Vec2::operator -= (float f) {
//	float fInv = 1.0f / f;
//	x -= fInv;
//	y -= fInv;
//}
//
//void	Vec2::operator *= (float f) {
//	x *= f;
//	y *= f;
//}
//
//void	Vec2::operator /= (float f) {
//	float fInv = 1.0f / f;
//	x *= fInv;
//	y *= fInv;
//}

// ----------------------------------------------------------------------------
//// unary operators
//// ----------------------------------------------------------------------------
//
//Vec2 Vec2::operator + () const {
//	return *this;
//}
//
//Vec2 Vec2::operator - () const {
//	return Vec2 (-x, -y);
//}

// ----------------------------------------------------------------------------
// binary operators
// ----------------------------------------------------------------------------

//Vec2 Vec2::operator + (const Vec2 &v) const {
//	return Vec2 (x + v.x, y + v.y);
//}
//
//Vec2 Vec2::operator - (const Vec2 &v) const {
//	return Vec2 (x - v.x, y - v.y);
//}
//
//Vec2 Vec2::operator * (const Vec2 &v) const {
//	return Vec2 (x * v.x, y * v.y);
//}
//
//Vec2 Vec2::operator / (const Vec2 &v) const {
//	return Vec2 (x / v.x, y / v.y);
//}
//
//Vec2 Vec2::operator + (float f) const {
//	return Vec2 (x + f, y + f);
//}
//
//Vec2 Vec2::operator - (float f) const {
//	return Vec2 (x - f, y - f);
//}
//
//Vec2 Vec2::operator * (float f) const {
//	return Vec2 (x * f, y * f);
//}
//
//Vec2 Vec2::operator / (float f) const {
//	float fInv = 1.0F / f;
//	return Vec2 (x * fInv, y * fInv);
//}

// ----------------------------------------------------------------------------

//Vec2 operator + (float f, const Vec2 &v) {
//	return Vec2 (f + v.x, f + v.y);
//}
//
//Vec2 operator - (float f, const Vec2 &v) {
//	return Vec2 (f - v.x, f - v.y);
//}
//
//Vec2 operator * (float f, const Vec2 &v) {
//	return Vec2 (f * v.x, f * v.y);
//}

// ----------------------------------------------------------------------------
/*
bool	Vec2::operator == (const Vec2 &v) const {
	return ((v.x == x) && (v.y == y));
}

bool	Vec2::operator != (const Vec2 &v) const {
	return ((v.x != x) || (v.y != y));
}
*/

//bool	Vec2::operator == (const Vec2 &v) const {
//	Vec2 d = v - *this;
//	const float f = FLOAT_EQUALITY_FUDGE;
//	if (d.x < -f) return false;
//	if (d.x > f) return false;
//	if (d.y < -f) return false;
//	if (d.y > f) return false;
//	return true;
//}
//
//bool	Vec2::operator != (const Vec2 &v) const {
//	Vec3 d = v - *this;
//	const float f = FLOAT_EQUALITY_FUDGE;
//	if (d.x < -f) return true;
//	if (d.x > f) return true;
//	if (d.y < -f) return true;
//	if (d.y > f) return true;
//	return false;
//}

// ----------------------------------------------------------------------------
// functions
// ----------------------------------------------------------------------------

//float Vec2::Length() const
//{
//	return sqrtf(x * x + y * y);
//}
//
//float Vec2::Length2() const
//{
//	return (x * x + y * y);
//}
//
//float Vec2::Normalize ()
//{
//	float length = sqrtf(x * x + y * y);
//	if (length != 0) {
//		float k = 1.0f / length;
//		x *= k;
//		y *= k;
//	}
//	return length;
//}
