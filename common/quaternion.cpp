#include "quaternion.h"
#include "matrix.h"

using namespace mr;

const Quaternion Quaternion::Identity(0.f, 0.f, 0.f, 1.f);

//// ----------------------------------------------------------------------------
//// static functions
//// ----------------------------------------------------------------------------
//
//M_INLINE
//void QuaternionMultiply(Quaternion *out, const Quaternion *a, const Quaternion *b)
//{
//	Quaternion qT;
//	qT.x = a->x * b->w + b->x * a->w + a->y * b->z - a->z * b->y;
//	qT.y = a->y * b->w + b->y * a->w + a->z * b->x - a->x * b->z;
//	qT.z = a->z * b->w + b->z * a->w + a->x * b->y - a->y * b->x;
//	qT.w = a->w * b->w - a->x * b->x - a->y * b->y - a->z * b->z;
//	*out = qT;
//}
//
//// ----------------------------------------------------------------------------
//// assignment operators
//// ----------------------------------------------------------------------------
//
//Quaternion& Quaternion::operator += (const Quaternion& q)
//{
//    x += q.x;
//    y += q.y;
//    z += q.z;
//    w += q.w;
//    return *this;
//}
//
//Quaternion& Quaternion::operator -= (const Quaternion& q)
//{
//    x -= q.x;
//    y -= q.y;
//    z -= q.z;
//    w -= q.w;
//    return *this;
//}
//
//Quaternion& Quaternion::operator *= (const Quaternion& q)
//{
//    QuaternionMultiply(this, &q, this);
//    return *this;
//}
//
//Quaternion& Quaternion::operator *= (float f)
//{
//    x *= f;
//    y *= f;
//    z *= f;
//    w *= f;
//    return *this;
//}
//
//Quaternion& Quaternion::operator /= (float f)
//{
//    float fInv = 1.f / f;
//    x *= fInv;
//    y *= fInv;
//    z *= fInv;
//    w *= fInv;
//    return *this;
//}
//
//// ----------------------------------------------------------------------------
//// unary operators
//// ----------------------------------------------------------------------------
//
//Quaternion  Quaternion::operator + () const
//{
//    return *this;
//}
//
//Quaternion  Quaternion::operator - () const
//{
//    return Quaternion(-x, -y, -z, -w);
//}
//
//// ----------------------------------------------------------------------------
//// binary operators
//// ----------------------------------------------------------------------------
//
//Quaternion Quaternion::operator + (const Quaternion& q) const
//{
//    return Quaternion(x + q.x, y + q.y, z + q.z, w + q.w);
//}
//
//Quaternion Quaternion::operator - (const Quaternion& q) const
//{
//    return Quaternion(x - q.x, y - q.y, z - q.z, w - q.w);
//}
//
//Quaternion Quaternion::operator * (const Quaternion& q) const
//{
//    Quaternion qT;
//    QuaternionMultiply(&qT, this, &q);
//    return qT;
//}
//
//Quaternion Quaternion::operator * (float f) const
//{
//    return Quaternion(x * f, y * f, z * f, w * f);
//}
//
//Quaternion Quaternion::operator / (float f) const
//{
//    float fInv = 1.f / f;
//    return Quaternion(x * fInv, y * fInv, z * fInv, w * fInv);
//}
//
//// ----------------------------------------------------------------------------
//
//Quaternion operator * (float f, const Quaternion& q)
//{
//    return Quaternion(f * q.x, f * q.y, f * q.z, f * q.w);
//}
//
//// ----------------------------------------------------------------------------
//
//bool Quaternion::operator == (const Quaternion& q) const
//{
//    return x == q.x && y == q.y && z == q.z && w == q.w;
//}
//
//bool Quaternion::operator != (const Quaternion& q) const
//{
//    return x != q.x || y != q.y || z != q.z || w != q.w;
//}
//
//// ----------------------------------------------------------------------------
//// functions
//// ----------------------------------------------------------------------------
//
//void Quaternion::SetIdentity()
//{
//	x = y = z = 0.f;
//	w = 1.f;
//}
//
//float Quaternion::Length2()
//{
//	return x*x + y*y + z*z + w*w;
//}
//
//float Quaternion::Length()
//{
//	return sqrtf(x*x + y*y + z*z + w*w);
//}
//
//float Quaternion::Normalize()
//{
//	float	length = Length();
//	if (length != 0) {
//		float f = 1.f / length;
//		x *= f;
//		y *= f;
//		z *= f;
//		w *= f;
//	}
//	return length;
//}
//
//void Quaternion::Inverse()
//{
//	*this /= -Length2();
//}
//
//void Quaternion::Conjugate()
//{
//	*this = Quaternion(-x, -y, -z, -w);
//}

void Quaternion::Slerp(const Quaternion &q1, const Quaternion &q2, float t)
{
	// Calculate the cosine of the angle between the two
	float scale0, scale1;
	float cosinus = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
	bool flip = (cosinus < 0.f);

	if (flip)
		cosinus = -cosinus;

	// If the angle is significant, use the spherical interpolation
	if ((1 - cosinus) > 0.00001){
		float temp  = acosf(cosinus);
		float sinus1 = 1.f / sinf(temp);
		scale0 = sinf((1 - t) * temp) * sinus1;
		scale1 = sinf(t * temp) * sinus1;
	}else{  //  Else use the cheaper linear interpolation
		scale0 = 1 - t;
		scale1 = t;
	}

	// the interpolated result
	*this = (q1 * scale0) + (q2 * (flip ? -scale1 : scale1));
}

void Quaternion::RotationYawPitchRoll(float yaw, float pitch, float roll)
{
	float	angle;
	float	sr, sp, sy, cr, cp, cy;

	angle = roll * 0.5f;
	sy = sinf(angle);
	cy = cosf(angle);
	angle = pitch * 0.5f;
	sp = sinf(angle);
	cp = cosf(angle);
	angle = yaw * 0.5f;
	sr = sinf(angle);
	cr = cosf(angle);

	x = cr*sp*cy+sr*cp*sy; // X
	y = sr*cp*cy-cr*sp*sy; // Y
	z = cr*cp*sy-sr*sp*cy; // Z
	w = cr*cp*cy+sr*sp*sy; // W
}

void Quaternion::RotationAxis(const Vec3 &vAxis, float angle)
{
	float d = angle * 0.5f;
	float s = sinf(d);
	x = vAxis.x * s;
	y = vAxis.y * s;
	z = vAxis.z * s;
	w = cosf(d);
}

void Quaternion::GetAxisAngle(Vec3 &vAxis, float &angle)
{
	angle = acosf(w);
	float f = 1.f / sinf(angle);
	vAxis.x = x * f;
	vAxis.y = y * f;
	vAxis.z = z * f;
	angle *= 2.f;
}

void Quaternion::RotationMatrix(const Matrix &mat)
{
	// Check the sum of the diagonal
	float tr = mat.m[0][0] + mat.m[1][1] + mat.m[2][2];
	if (tr > 0.f) {
		// The sum is positive
		// 4 muls][1 div][6 adds][1 trig function call
		float s = sqrtf(tr + 1);
		w = s * 0.5f;
		s    = 0.5f / s;
		x = (mat.m[1][2] - mat.m[2][1]) * s;
		y = (mat.m[2][0] - mat.m[0][2]) * s;
		z = (mat.m[0][1] - mat.m[1][0]) * s;
	} else {
		// The sum is negative
		// 4 muls][1 div][8 adds][1 trig function call
		const int nIndex[3] = {1, 2, 0};
		int i, j, k;
		i = 0;
		if( mat.m[1][1] > mat.m[i][i] ) i = 1;
		if( mat.m[2][2] > mat.m[i][i] ) i = 2;
		j = nIndex[i];
		k = nIndex[j];

		float s = sqrtf((mat.m[i][i] - (mat.m[j][j] + mat.m[k][k])) + 1);
		((float *)this)[i] = s * 0.5f;
		if (s != 0.f) s = 0.5f / s;
		((float *)this)[j] = (mat.m[i][j] + mat.m[j][i]) * s;
		((float *)this)[k] = (mat.m[i][k] + mat.m[k][i]) * s;
		w = (mat.m[j][k] - mat.m[k][j]) * s;
	}
}