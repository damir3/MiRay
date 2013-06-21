#pragma once

#include "vec3.h"
#include "quaternion.h"

namespace mr
{

void MatrixMultiply(Matrix & out, const Matrix & a, const Matrix & b);

struct Matrix
{
	union {
		struct {
			float m11, m12, m13, m14;
			float m21, m22, m23, m24;
			float m31, m32, m33, m34;
			float m41, m42, m43, m44;
		};
		float m[4][4];
	};

	Matrix() {}
	Matrix(const Matrix & mat)
	{
		::memcpy(m, mat.m, sizeof(m));
	}
	Matrix(const float mat[16])
	{
		::memcpy(m, mat, sizeof(m));
	}
	Matrix( float f11, float f12, float f13, float f14,
		float f21, float f22, float f23, float f24,
		float f31, float f32, float f33, float f34,
		float f41, float f42, float f43, float f44)
	{
		m11 = f11; m12 = f12; m13 = f13; m14 = f14;
		m21 = f21; m22 = f22; m23 = f23; m24 = f24;
		m31 = f31; m32 = f32; m33 = f33; m34 = f34;
		m41 = f41; m42 = f42; m43 = f43; m44 = f44;
	}

	// access grants
	float& operator () (int iRow, int iCol) {return m[iRow][iCol];}
	float  operator () (int iRow, int iCol) const {return m[iRow][iCol];}

	// casting operators
	operator float* () {return &m11;}
	operator const float* () const {return &m11;}

	// assignment operators
	Matrix& operator *= (const Matrix& mat)
	{
		MatrixMultiply(*this, *this, mat);
		return *this;
	}
	Matrix& operator += (const Matrix& mat)
	{
		m11 += mat.m11; m12 += mat.m12; m13 += mat.m13; m14 += mat.m14;
		m21 += mat.m21; m22 += mat.m22; m23 += mat.m23; m24 += mat.m24;
		m31 += mat.m31; m32 += mat.m32; m33 += mat.m33; m34 += mat.m34;
		m41 += mat.m41; m42 += mat.m42; m43 += mat.m43; m44 += mat.m44;
		return *this;
	}
	Matrix& operator -= (const Matrix& mat)
	{
		m11 -= mat.m11; m12 -= mat.m12; m13 -= mat.m13; m14 -= mat.m14;
		m21 -= mat.m21; m22 -= mat.m22; m23 -= mat.m23; m24 -= mat.m24;
		m31 -= mat.m31; m32 -= mat.m32; m33 -= mat.m33; m34 -= mat.m34;
		m41 -= mat.m41; m42 -= mat.m42; m43 -= mat.m43; m44 -= mat.m44;
		return *this;
	}
	Matrix& operator *= (float f)
	{
		m11 *= f; m12 *= f; m13 *= f; m14 *= f;
		m21 *= f; m22 *= f; m23 *= f; m24 *= f;
		m31 *= f; m32 *= f; m33 *= f; m34 *= f;
		m41 *= f; m42 *= f; m43 *= f; m44 *= f;
		return *this;
	}
	Matrix& operator /= (float f)
	{
		float fInv = 1.f / f;
		m11 *= fInv; m12 *= fInv; m13 *= fInv; m14 *= fInv;
		m21 *= fInv; m22 *= fInv; m23 *= fInv; m24 *= fInv;
		m31 *= fInv; m32 *= fInv; m33 *= fInv; m34 *= fInv;
		m41 *= fInv; m42 *= fInv; m43 *= fInv; m44 *= fInv;
		return *this;
	}

	// unary operators
	Matrix operator + () const
	{
		return *this;
	}
	Matrix operator - () const
	{
		return	Matrix( -m11, -m12, -m13, -m14,
			-m21, -m22, -m23, -m24,
			-m31, -m32, -m33, -m34,
			-m41, -m42, -m43, -m44 );
	}

	// binary operators
	Matrix operator * (const Matrix& mat) const
	{
		Matrix matT;
		MatrixMultiply(matT, *this, mat);
		return matT;
	}
	Matrix operator + (const Matrix& mat) const
	{
		return	Matrix( m11 + mat.m11, m12 + mat.m12, m13 + mat.m13, m14 + mat.m14,
			m21 + mat.m21, m22 + mat.m22, m23 + mat.m23, m24 + mat.m24,
			m31 + mat.m31, m32 + mat.m32, m33 + mat.m33, m34 + mat.m34,
			m41 + mat.m41, m42 + mat.m42, m43 + mat.m43, m44 + mat.m44 );
	}
	Matrix operator - (const Matrix& mat) const
	{
		return	Matrix( m11 - mat.m11, m12 - mat.m12, m13 - mat.m13, m14 - mat.m14,
			m21 - mat.m21, m22 - mat.m22, m23 - mat.m23, m24 - mat.m24,
			m31 - mat.m31, m32 - mat.m32, m33 - mat.m33, m34 - mat.m34,
			m41 - mat.m41, m42 - mat.m42, m43 - mat.m43, m44 - mat.m44 );
	}
	Matrix operator * (float f) const
	{
		return	Matrix( m11 * f, m12 * f, m13 * f, m14 * f,
			m21 * f, m22 * f, m23 * f, m24 * f,
			m31 * f, m32 * f, m33 * f, m34 * f,
			m41 * f, m42 * f, m43 * f, m44 * f );
	}
	Matrix operator / (float f) const
	{
		float fInv = 1.f / f;
		return	Matrix( m11 * fInv, m12 * fInv, m13 * fInv, m14 * fInv,
			m21 * fInv, m22 * fInv, m23 * fInv, m24 * fInv,
			m31 * fInv, m32 * fInv, m33 * fInv, m34 * fInv,
			m41 * fInv, m42 * fInv, m43 * fInv, m44 * fInv );
	}

	friend Matrix operator * (float f, const Matrix& mat)
	{
		return	Matrix( f * mat.m11, f * mat.m12, f * mat.m13, f * mat.m14,
			f * mat.m21, f * mat.m22, f * mat.m23, f * mat.m24,
			f * mat.m31, f * mat.m32, f * mat.m33, f * mat.m34,
			f * mat.m41, f * mat.m42, f * mat.m43, f * mat.m44 );
	}

	bool operator == (const Matrix& mat) const
	{
		return 0 == ::memcmp(m, &mat.m, sizeof(m));
	}
	bool operator != (const Matrix& mat) const
	{
		return 0 != ::memcmp(m, &mat.m, sizeof(m));
	}

	Vec3 operator * (const Vec3&) const;

	const Vec3 & Axis(int i) const {return *(Vec3*)m[i];}
	const Vec3 & AxisX() const {return Axis(0);}
	const Vec3 & AxisY() const {return Axis(1);}
	const Vec3 & AxisZ() const {return Axis(2);}
	const Vec3 & Pos() const {return Axis(3);}
	Vec3 Rotation() const;
	Vec3 Scale() const { return Vec3(AxisX().Length(), AxisY().Length(), AxisZ().Length()); }

	Vec3 & Axis(int i) {return *(Vec3*)m[i];}
	Vec3 & AxisX() {return Axis(0);}
	Vec3 & AxisY() {return Axis(1);}
	Vec3 & AxisZ() {return Axis(2);}
	Vec3 & Pos() {return Axis(3);}

	// functions
	void SetIdentity();
	void Transpose(const Matrix&);
	void Transpose();
	void Inverse(const Matrix&);
	void Translation(const Vec3&);
	void Scale(const Vec3&);
	void RotationAxis(const Vec3 &axis, float angle_rad, const Vec3 & point = Vec3::Null);
	void RotationQuaternion(const Quaternion &q);
	void RotationYawPitchRoll(float yaw, float pitch, float roll);
	//void Reflect(const sPlane &plane);
	void Orthonormalize();
	void Transformation(
		const Vec3& vScalingCenter,
		const Quaternion& qScalingRotation,
		const Vec3& vScaling,
		const Vec3& vRotationCenter,
		const Quaternion& qRotation,
		const Vec3& vTranslation);
	
	void SetFrustum(float left, float right, float bottom, float top, float near, float far);

	// -------------------------------------------------------------------- //

	static const Matrix Identity;
	static const Matrix CubemapPosX;
	static const Matrix CubemapNegX;
	static const Matrix CubemapPosY;
	static const Matrix CubemapNegY;
	static const Matrix CubemapPosZ;
	static const Matrix CubemapNegZ;
};


//#define	MatrixDet3(mat, iRow1, iRow2, iRow3, iCol1, iCol2, iCol3) (mat.m[iRow1][iCol1] * (mat.m[iRow2][iCol2]*mat.m[iRow3][iCol3] - mat.m[iRow3][iCol2]*mat.m[iRow2][iCol3]) - mat.m[iRow1][iCol2] * (mat.m[iRow2][iCol1]*mat.m[iRow3][iCol3] - mat.m[iRow3][iCol1]*mat.m[iRow2][iCol3]) + mat.m[iRow1][iCol3] * (mat.m[iRow2][iCol1]*mat.m[iRow3][iCol2] - mat.m[iRow3][iCol1]*mat.m[iRow2][iCol2]))
//
//M_INLINE
//void Matrix::Inverse (const Matrix& mat)
//{
//	Matrix matTmp;
//	matTmp.m11 =  MatrixDet3(mat, 1, 2, 3, 1, 2, 3);
//	matTmp.m21 = -MatrixDet3(mat, 1, 2, 3, 0, 2, 3);
//	matTmp.m31 =  MatrixDet3(mat, 1, 2, 3, 0, 1, 3);
//	matTmp.m41 = -MatrixDet3(mat, 1, 2, 3, 0, 1, 2);
//	matTmp.m12 = -MatrixDet3(mat, 0, 2, 3, 1, 2, 3);
//	matTmp.m22 =  MatrixDet3(mat, 0, 2, 3, 0, 2, 3);
//	matTmp.m32 = -MatrixDet3(mat, 0, 2, 3, 0, 1, 3);
//	matTmp.m42 =  MatrixDet3(mat, 0, 2, 3, 0, 1, 2);
//	matTmp.m13 =  MatrixDet3(mat, 0, 1, 3, 1, 2, 3);
//	matTmp.m23 = -MatrixDet3(mat, 0, 1, 3, 0, 2, 3);
//	matTmp.m33 =  MatrixDet3(mat, 0, 1, 3, 0, 1, 3);
//	matTmp.m43 = -MatrixDet3(mat, 0, 1, 3, 0, 1, 2);
//	matTmp.m14 = -MatrixDet3(mat, 0, 1, 2, 1, 2, 3);
//	matTmp.m24 =  MatrixDet3(mat, 0, 1, 2, 0, 2, 3);
//	matTmp.m34 = -MatrixDet3(mat, 0, 1, 2, 0, 1, 3);
//	matTmp.m44 =  MatrixDet3(mat, 0, 1, 2, 0, 1, 2);
//	float det = mat.m11 * matTmp.m11 + mat.m12 * matTmp.m21 + mat.m13 * matTmp.m31 + mat.m14 * matTmp.m41;
//	if (fabsf(det) < 1e-5) return;
//	*this = matTmp / det;
//}

}