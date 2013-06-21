#include "vec4.h"

using namespace mr;

// ----------------------------------------------------------------------------
// static const variables
// ----------------------------------------------------------------------------

template<> const Vec4F Vec4F::Null(0.f, 0.f, 0.f, 0.f);

// ----------------------------------------------------------------------------
// functions
// ----------------------------------------------------------------------------

template<>
void Vec4::Transform(const Matrix &mat)
{
	Vec4 tmp = *(this);
	x = tmp.x*mat.m11 + tmp.y*mat.m21 + tmp.z*mat.m31 + tmp.w*mat.m41;
	y = tmp.x*mat.m12 + tmp.y*mat.m22 + tmp.z*mat.m32 + tmp.w*mat.m42;
	z = tmp.x*mat.m13 + tmp.y*mat.m23 + tmp.z*mat.m33 + tmp.w*mat.m43;
	w = tmp.x*mat.m14 + tmp.y*mat.m24 + tmp.z*mat.m34 + tmp.w*mat.m44;
}

template<>
void Vec4::TTransform(const Matrix &mat)
{
	Vec4 tmp = *(this);
	x = tmp.x*mat.m11 + tmp.y*mat.m12 + tmp.z*mat.m13 + tmp.w*mat.m14;
	y = tmp.x*mat.m21 + tmp.y*mat.m22 + tmp.z*mat.m23 + tmp.w*mat.m24;
	z = tmp.x*mat.m31 + tmp.y*mat.m32 + tmp.z*mat.m33 + tmp.w*mat.m34;
	w = tmp.x*mat.m41 + tmp.y*mat.m42 + tmp.z*mat.m43 + tmp.w*mat.m44;
}
