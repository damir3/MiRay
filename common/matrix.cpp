#include "matrix.h"

using namespace mr;

// ----------------------------------------------------------------------------

const Matrix Matrix::Identity(1, 0, 0, 0,
							  0, 1, 0, 0,
							  0, 0, 1, 0,
							  0, 0, 0, 1);

const Matrix Matrix::CubemapPosX(0, 0, -1, 0, +1, 0, 0, 0, 0, +1, 0, 0, 0, 0, 0, 1);
const Matrix Matrix::CubemapNegX(0, 0, +1, 0, -1, 0, 0, 0, 0, +1, 0, 0, 0, 0, 0, 1);
const Matrix Matrix::CubemapPosY(+1, 0, 0, 0, 0, +1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1);
const Matrix Matrix::CubemapNegY(+1, 0, 0, 0, 0, -1, 0, 0, 0, 0, +1, 0, 0, 0, 0, 1);
const Matrix Matrix::CubemapPosZ(+1, 0, 0, 0, 0, 0, +1, 0, 0, +1, 0, 0, 0, 0, 0, 1);
const Matrix Matrix::CubemapNegZ(-1, 0, 0, 0, 0, 0, -1, 0, 0, +1, 0, 0, 0, 0, 0, 1);

// ----------------------------------------------------------------------------
// static functions
// ----------------------------------------------------------------------------

//void __fastcall MatrixMultiply (Matrix & out, const Matrix & a, const Matrix & b)
//{
//	Matrix mat;
//	Matrix & target = (&out != &a && &out !	= &b) ? out : mat;
//
//	target.m11 = a.m11*b.m11 + a.m12*b.m21 + a.m13*b.m31 + a.m14*b.m41;
//	target.m12 = a.m11*b.m12 + a.m12*b.m22 + a.m13*b.m32 + a.m14*b.m42;
//	target.m13 = a.m11*b.m13 + a.m12*b.m23 + a.m13*b.m33 + a.m14*b.m43;
//	target.m14 = a.m11*b.m14 + a.m12*b.m24 + a.m13*b.m34 + a.m14*b.m44;
//
//	target.m21 = a.m21*b.m11 + a.m22*b.m21 + a.m23*b.m31 + a.m24*b.m41;
//	target.m22 = a.m21*b.m12 + a.m22*b.m22 + a.m23*b.m32 + a.m24*b.m42;
//	target.m23 = a.m21*b.m13 + a.m22*b.m23 + a.m23*b.m33 + a.m24*b.m43;
//	target.m24 = a.m21*b.m14 + a.m22*b.m24 + a.m23*b.m34 + a.m24*b.m44;
//
//	target.m31 = a.m31*b.m11 + a.m32*b.m21 + a.m33*b.m31 + a.m34*b.m41;
//	target.m32 = a.m31*b.m12 + a.m32*b.m22 + a.m33*b.m32 + a.m34*b.m42;
//	target.m33 = a.m31*b.m13 + a.m32*b.m23 + a.m33*b.m33 + a.m34*b.m43;
//	target.m34 = a.m31*b.m14 + a.m32*b.m24 + a.m33*b.m34 + a.m34*b.m44;
//
//	target.m41 = a.m41*b.m11 + a.m42*b.m21 + a.m43*b.m31 + a.m44*b.m41;
//	target.m42 = a.m41*b.m12 + a.m42*b.m22 + a.m43*b.m32 + a.m44*b.m42;
//	target.m43 = a.m41*b.m13 + a.m42*b.m23 + a.m43*b.m33 + a.m44*b.m43;
//	target.m44 = a.m41*b.m14 + a.m42*b.m24 + a.m43*b.m34 + a.m44*b.m44;
//
//	if (&out != &target) out = target;
//}

//inline static float	MatrixDet3 (const Matrix &mat, int iRow1, int iRow2, int iRow3, int iCol1, int iCol2, int iCol3) {
//	return mat.m[iRow1][iCol1] * (mat.m[iRow2][iCol2]*mat.m[iRow3][iCol3] - mat.m[iRow3][iCol2]*mat.m[iRow2][iCol3]) -
//		mat.m[iRow1][iCol2] * (mat.m[iRow2][iCol1]*mat.m[iRow3][iCol3] - mat.m[iRow3][iCol1]*mat.m[iRow2][iCol3]) +
//		mat.m[iRow1][iCol3] * (mat.m[iRow2][iCol1]*mat.m[iRow3][iCol2] - mat.m[iRow3][iCol1]*mat.m[iRow2][iCol2]);
//}

#define	MatrixDet3(mat, iRow1, iRow2, iRow3, iCol1, iCol2, iCol3) (mat.m[iRow1][iCol1] * (mat.m[iRow2][iCol2]*mat.m[iRow3][iCol3] - mat.m[iRow3][iCol2]*mat.m[iRow2][iCol3]) - mat.m[iRow1][iCol2] * (mat.m[iRow2][iCol1]*mat.m[iRow3][iCol3] - mat.m[iRow3][iCol1]*mat.m[iRow2][iCol3]) + mat.m[iRow1][iCol3] * (mat.m[iRow2][iCol1]*mat.m[iRow3][iCol2] - mat.m[iRow3][iCol1]*mat.m[iRow2][iCol2]))

// ----------------------------------------------------------------------------
// access grants
// ----------------------------------------------------------------------------

//float& Matrix::operator () (int iRow, int iCol)
//{
//	return m[iRow][iCol];
//}
//
//float Matrix::operator () (int iRow, int iCol) const
//{
//	return m[iRow][iCol];
//}

// ----------------------------------------------------------------------------
// casting operators
// ----------------------------------------------------------------------------

//Matrix::operator float* ()
//{
//	return (float *) &m11;
//}
//
//Matrix::operator const float* () const
//{
//	return (const float *) &m11;
//}

// ----------------------------------------------------------------------------
// assignment operators
// ----------------------------------------------------------------------------

//Matrix& Matrix::operator *= (const Matrix& mat)
//{
//	MatrixMultiply(*this, *this, mat);
//	return *this;
//}
//
//Matrix& Matrix::operator += (const Matrix& mat)
//{
//	m11 += mat.m11; m12 += mat.m12; m13 += mat.m13; m14 += mat.m14;
//	m21 += mat.m21; m22 += mat.m22; m23 += mat.m23; m24 += mat.m24;
//	m31 += mat.m31; m32 += mat.m32; m33 += mat.m33; m34 += mat.m34;
//	m41 += mat.m41; m42 += mat.m42; m43 += mat.m43; m44 += mat.m44;
//	return *this;
//}
//
//Matrix& Matrix::operator -= (const Matrix& mat)
//{
//	m11 -= mat.m11; m12 -= mat.m12; m13 -= mat.m13; m14 -= mat.m14;
//	m21 -= mat.m21; m22 -= mat.m22; m23 -= mat.m23; m24 -= mat.m24;
//	m31 -= mat.m31; m32 -= mat.m32; m33 -= mat.m33; m34 -= mat.m34;
//	m41 -= mat.m41; m42 -= mat.m42; m43 -= mat.m43; m44 -= mat.m44;
//	return *this;
//}
//
//Matrix& Matrix::operator *= (float f)
//{
//	m11 *= f; m12 *= f; m13 *= f; m14 *= f;
//	m21 *= f; m22 *= f; m23 *= f; m24 *= f;
//	m31 *= f; m32 *= f; m33 *= f; m34 *= f;
//	m41 *= f; m42 *= f; m43 *= f; m44 *= f;
//	return *this;
//}
//
//Matrix& Matrix::operator /= (float f)
//{
//	float fInv = 1.f / f;
//	m11 *= fInv; m12 *= fInv; m13 *= fInv; m14 *= fInv;
//	m21 *= fInv; m22 *= fInv; m23 *= fInv; m24 *= fInv;
//	m31 *= fInv; m32 *= fInv; m33 *= fInv; m34 *= fInv;
//	m41 *= fInv; m42 *= fInv; m43 *= fInv; m44 *= fInv;
//	return *this;
//}

// ----------------------------------------------------------------------------
// unary operators
// ----------------------------------------------------------------------------

//Matrix Matrix::operator + () const
//{
//	return *this;
//}
//
//Matrix Matrix::operator - () const
//{
//	return Matrix(-m11, -m12, -m13, -m14,
//		-m21, -m22, -m23, -m24,
//		-m31, -m32, -m33, -m34,
//		-m41, -m42, -m43, -m44);
//}

// ----------------------------------------------------------------------------
// binary operators
// ----------------------------------------------------------------------------

//Matrix Matrix::operator * (const Matrix& mat) const
//{
//	Matrix matT;
//	MatrixMultiply(matT, *this, mat);
//	return matT;
//}
//
//Matrix Matrix::operator + (const Matrix& mat) const
//{
//	return Matrix(m11 + mat.m11, m12 + mat.m12, m13 + mat.m13, m14 + mat.m14,
//				m21 + mat.m21, m22 + mat.m22, m23 + mat.m23, m24 + mat.m24,
//				m31 + mat.m31, m32 + mat.m32, m33 + mat.m33, m34 + mat.m34,
//				m41 + mat.m41, m42 + mat.m42, m43 + mat.m43, m44 + mat.m44);
//}
//
//Matrix Matrix::operator - (const Matrix& mat) const
//{
//	return Matrix(m11 - mat.m11, m12 - mat.m12, m13 - mat.m13, m14 - mat.m14,
//				m21 - mat.m21, m22 - mat.m22, m23 - mat.m23, m24 - mat.m24,
//				m31 - mat.m31, m32 - mat.m32, m33 - mat.m33, m34 - mat.m34,
//				m41 - mat.m41, m42 - mat.m42, m43 - mat.m43, m44 - mat.m44);
//}
//
//Matrix Matrix::operator * (float f) const
//{
//	return Matrix(m11 * f, m12 * f, m13 * f, m14 * f,
//				m21 * f, m22 * f, m23 * f, m24 * f,
//				m31 * f, m32 * f, m33 * f, m34 * f,
//				m41 * f, m42 * f, m43 * f, m44 * f);
//}
//
//Matrix Matrix::operator / (float f) const
//{
//	float fInv = 1.f / f;
//	return Matrix(m11 * fInv, m12 * fInv, m13 * fInv, m14 * fInv,
//				m21 * fInv, m22 * fInv, m23 * fInv, m24 * fInv,
//				m31 * fInv, m32 * fInv, m33 * fInv, m34 * fInv,
//				m41 * fInv, m42 * fInv, m43 * fInv, m44 * fInv);
//}

// ----------------------------------------------------------------------------

//Matrix operator * (float f, const Matrix& mat)
//{
//	return Matrix(f * mat.m11, f * mat.m12, f * mat.m13, f * mat.m14,
//				f * mat.m21, f * mat.m22, f * mat.m23, f * mat.m24,
//				f * mat.m31, f * mat.m32, f * mat.m33, f * mat.m34,
//				f * mat.m41, f * mat.m42, f * mat.m43, f * mat.m44);
//}

// ----------------------------------------------------------------------------

//bool Matrix::operator == (const Matrix& mat) const
//{
//	return 0 == ::memcmp(m, &mat.m, sizeof(m));
//}
//
//bool Matrix::operator != (const Matrix& mat) const
//{
//	return 0 != ::memcmp(m, &mat.m, sizeof(m));
//}

// ----------------------------------------------------------------------------
// functions
// ----------------------------------------------------------------------------

void Matrix::SetIdentity()
{
	::memset(m, 0, sizeof(m));
	m11 = m22 = m33 = m44 = 1.f;
}

void Matrix::Transpose(const Matrix &mat)
{
	if (this != &mat) {
		m11 = mat.m11; m12 = mat.m21; m13 = mat.m31; m14 = mat.m41;
		m21 = mat.m12; m22 = mat.m22; m23 = mat.m32; m24 = mat.m42;
		m31 = mat.m13; m32 = mat.m23; m33 = mat.m33; m34 = mat.m43;
		m41 = mat.m14; m42 = mat.m24; m43 = mat.m34; m44 = mat.m44;
	} else {
		Transpose();
	}
}

void Matrix::Transpose()
{
	std::swap(m12, m21);
	std::swap(m13, m31);
	std::swap(m14, m41);
	std::swap(m23, m32);
	std::swap(m24, m42);
	std::swap(m34, m43);
}

void Matrix::Inverse(const Matrix& mat)
{
	Matrix matTmp;
	matTmp.m11 =  MatrixDet3(mat, 1, 2, 3, 1, 2, 3);
	matTmp.m21 = -MatrixDet3(mat, 1, 2, 3, 0, 2, 3);
	matTmp.m31 =  MatrixDet3(mat, 1, 2, 3, 0, 1, 3);
	matTmp.m41 = -MatrixDet3(mat, 1, 2, 3, 0, 1, 2);
	matTmp.m12 = -MatrixDet3(mat, 0, 2, 3, 1, 2, 3);
	matTmp.m22 =  MatrixDet3(mat, 0, 2, 3, 0, 2, 3);
	matTmp.m32 = -MatrixDet3(mat, 0, 2, 3, 0, 1, 3);
	matTmp.m42 =  MatrixDet3(mat, 0, 2, 3, 0, 1, 2);
	matTmp.m13 =  MatrixDet3(mat, 0, 1, 3, 1, 2, 3);
	matTmp.m23 = -MatrixDet3(mat, 0, 1, 3, 0, 2, 3);
	matTmp.m33 =  MatrixDet3(mat, 0, 1, 3, 0, 1, 3);
	matTmp.m43 = -MatrixDet3(mat, 0, 1, 3, 0, 1, 2);
	matTmp.m14 = -MatrixDet3(mat, 0, 1, 2, 1, 2, 3);
	matTmp.m24 =  MatrixDet3(mat, 0, 1, 2, 0, 2, 3);
	matTmp.m34 = -MatrixDet3(mat, 0, 1, 2, 0, 1, 3);
	matTmp.m44 =  MatrixDet3(mat, 0, 1, 2, 0, 1, 2);
	float det = mat.m11 * matTmp.m11 + mat.m12 * matTmp.m21 + mat.m13 * matTmp.m31 + mat.m14 * matTmp.m41;
	if (det == 0.f) return;
	*this = matTmp / det;

	// -------------------------------------------------------------------- //

	//// 84+4+16 = 104 multiplications
	////			   1 division
	//double det, invDet;

	//// 2x2 sub-determinants required to calculate 4x4 determinant
	//float det2_01_01 = mat.m[0][0] * mat.m[1][1] - mat.m[0][1] * mat.m[1][0];
	//float det2_01_02 = mat.m[0][0] * mat.m[1][2] - mat.m[0][2] * mat.m[1][0];
	//float det2_01_03 = mat.m[0][0] * mat.m[1][3] - mat.m[0][3] * mat.m[1][0];
	//float det2_01_12 = mat.m[0][1] * mat.m[1][2] - mat.m[0][2] * mat.m[1][1];
	//float det2_01_13 = mat.m[0][1] * mat.m[1][3] - mat.m[0][3] * mat.m[1][1];
	//float det2_01_23 = mat.m[0][2] * mat.m[1][3] - mat.m[0][3] * mat.m[1][2];

	//// 3x3 sub-determinants required to calculate 4x4 determinant
	//float det3_201_012 = mat.m[2][0] * det2_01_12 - mat.m[2][1] * det2_01_02 + mat.m[2][2] * det2_01_01;
	//float det3_201_013 = mat.m[2][0] * det2_01_13 - mat.m[2][1] * det2_01_03 + mat.m[2][3] * det2_01_01;
	//float det3_201_023 = mat.m[2][0] * det2_01_23 - mat.m[2][2] * det2_01_03 + mat.m[2][3] * det2_01_02;
	//float det3_201_123 = mat.m[2][1] * det2_01_23 - mat.m[2][2] * det2_01_13 + mat.m[2][3] * det2_01_12;

	//det = ( - det3_201_123 * mat.m[3][0] + det3_201_023 * mat.m[3][1] - det3_201_013 * mat.m[3][2] + det3_201_012 * mat.m[3][3] );

	//if ( fabs( det ) < MATRIX_INVERSE_EPSILON ) return;

	//invDet = 1.0f / det;

	//// remaining 2x2 sub-determinants
	//float det2_03_01 = mat.m[0][0] * mat.m[3][1] - mat.m[0][1] * mat.m[3][0];
	//float det2_03_02 = mat.m[0][0] * mat.m[3][2] - mat.m[0][2] * mat.m[3][0];
	//float det2_03_03 = mat.m[0][0] * mat.m[3][3] - mat.m[0][3] * mat.m[3][0];
	//float det2_03_12 = mat.m[0][1] * mat.m[3][2] - mat.m[0][2] * mat.m[3][1];
	//float det2_03_13 = mat.m[0][1] * mat.m[3][3] - mat.m[0][3] * mat.m[3][1];
	//float det2_03_23 = mat.m[0][2] * mat.m[3][3] - mat.m[0][3] * mat.m[3][2];

	//float det2_13_01 = mat.m[1][0] * mat.m[3][1] - mat.m[1][1] * mat.m[3][0];
	//float det2_13_02 = mat.m[1][0] * mat.m[3][2] - mat.m[1][2] * mat.m[3][0];
	//float det2_13_03 = mat.m[1][0] * mat.m[3][3] - mat.m[1][3] * mat.m[3][0];
	//float det2_13_12 = mat.m[1][1] * mat.m[3][2] - mat.m[1][2] * mat.m[3][1];
	//float det2_13_13 = mat.m[1][1] * mat.m[3][3] - mat.m[1][3] * mat.m[3][1];
	//float det2_13_23 = mat.m[1][2] * mat.m[3][3] - mat.m[1][3] * mat.m[3][2];

	//// remaining 3x3 sub-determinants
	//float det3_203_012 = mat.m[2][0] * det2_03_12 - mat.m[2][1] * det2_03_02 + mat.m[2][2] * det2_03_01;
	//float det3_203_013 = mat.m[2][0] * det2_03_13 - mat.m[2][1] * det2_03_03 + mat.m[2][3] * det2_03_01;
	//float det3_203_023 = mat.m[2][0] * det2_03_23 - mat.m[2][2] * det2_03_03 + mat.m[2][3] * det2_03_02;
	//float det3_203_123 = mat.m[2][1] * det2_03_23 - mat.m[2][2] * det2_03_13 + mat.m[2][3] * det2_03_12;

	//float det3_213_012 = mat.m[2][0] * det2_13_12 - mat.m[2][1] * det2_13_02 + mat.m[2][2] * det2_13_01;
	//float det3_213_013 = mat.m[2][0] * det2_13_13 - mat.m[2][1] * det2_13_03 + mat.m[2][3] * det2_13_01;
	//float det3_213_023 = mat.m[2][0] * det2_13_23 - mat.m[2][2] * det2_13_03 + mat.m[2][3] * det2_13_02;
	//float det3_213_123 = mat.m[2][1] * det2_13_23 - mat.m[2][2] * det2_13_13 + mat.m[2][3] * det2_13_12;

	//float det3_301_012 = mat.m[3][0] * det2_01_12 - mat.m[3][1] * det2_01_02 + mat.m[3][2] * det2_01_01;
	//float det3_301_013 = mat.m[3][0] * det2_01_13 - mat.m[3][1] * det2_01_03 + mat.m[3][3] * det2_01_01;
	//float det3_301_023 = mat.m[3][0] * det2_01_23 - mat.m[3][2] * det2_01_03 + mat.m[3][3] * det2_01_02;
	//float det3_301_123 = mat.m[3][1] * det2_01_23 - mat.m[3][2] * det2_01_13 + mat.m[3][3] * det2_01_12;

	//m[0][0] = (float)(- det3_213_123 * invDet);
	//m[1][0] = (float)(+ det3_213_023 * invDet);
	//m[2][0] = (float)(- det3_213_013 * invDet);
	//m[3][0] = (float)(+ det3_213_012 * invDet);

	//m[0][1] = (float)(+ det3_203_123 * invDet);
	//m[1][1] = (float)(- det3_203_023 * invDet);
	//m[2][1] = (float)(+ det3_203_013 * invDet);
	//m[3][1] = (float)(- det3_203_012 * invDet);

	//m[0][2] = (float)(+ det3_301_123 * invDet);
	//m[1][2] = (float)(- det3_301_023 * invDet);
	//m[2][2] = (float)(+ det3_301_013 * invDet);
	//m[3][2] = (float)(- det3_301_012 * invDet);

	//m[0][3] = (float)(- det3_201_123 * invDet);
	//m[1][3] = (float)(+ det3_201_023 * invDet);
	//m[2][3] = (float)(- det3_201_013 * invDet);
	//m[3][3] = (float)(+ det3_201_012 * invDet);

	// -------------------------------------------------------------------- //

	//typedef float Mat2[2][2];

	////	6*8+2*6 = 60 multiplications
	////		2*1 =  2 divisions
	//Mat2 r0, r1, r2, r3;
	//float a, det, invDet;
	////float *mat = reinterpret_cast<float *>(this);
	//const float * pMat = (const float *)mat;

	//// r0 = m0.Inverse();
	//det = pMat[0*4+0] * pMat[1*4+1] - pMat[0*4+1] * pMat[1*4+0];

	//if ( fabsf( det ) < MATRIX_INVERSE_EPSILON ) return;

	//invDet = 1.0f / det;

	//r0[0][0] =   pMat[1*4+1] * invDet;
	//r0[0][1] = - pMat[0*4+1] * invDet;
	//r0[1][0] = - pMat[1*4+0] * invDet;
	//r0[1][1] =   pMat[0*4+0] * invDet;

	//// r1 = r0 * m1;
	//r1[0][0] = r0[0][0] * pMat[0*4+2] + r0[0][1] * pMat[1*4+2];
	//r1[0][1] = r0[0][0] * pMat[0*4+3] + r0[0][1] * pMat[1*4+3];
	//r1[1][0] = r0[1][0] * pMat[0*4+2] + r0[1][1] * pMat[1*4+2];
	//r1[1][1] = r0[1][0] * pMat[0*4+3] + r0[1][1] * pMat[1*4+3];

	//// r2 = m2 * r1;
	//r2[0][0] = pMat[2*4+0] * r1[0][0] + pMat[2*4+1] * r1[1][0];
	//r2[0][1] = pMat[2*4+0] * r1[0][1] + pMat[2*4+1] * r1[1][1];
	//r2[1][0] = pMat[3*4+0] * r1[0][0] + pMat[3*4+1] * r1[1][0];
	//r2[1][1] = pMat[3*4+0] * r1[0][1] + pMat[3*4+1] * r1[1][1];

	//// r3 = r2 - m3;
	//r3[0][0] = r2[0][0] - pMat[2*4+2];
	//r3[0][1] = r2[0][1] - pMat[2*4+3];
	//r3[1][0] = r2[1][0] - pMat[3*4+2];
	//r3[1][1] = r2[1][1] - pMat[3*4+3];

	//// r3.InverseSelf();
	//det = r3[0][0] * r3[1][1] - r3[0][1] * r3[1][0];

	//if ( fabsf( det ) < MATRIX_INVERSE_EPSILON ) return;

	//invDet = 1.0f / det;

	//a = r3[0][0];
	//r3[0][0] =   r3[1][1] * invDet;
	//r3[0][1] = - r3[0][1] * invDet;
	//r3[1][0] = - r3[1][0] * invDet;
	//r3[1][1] =   a * invDet;

	//// r2 = m2 * r0;
	//r2[0][0] = pMat[2*4+0] * r0[0][0] + pMat[2*4+1] * r0[1][0];
	//r2[0][1] = pMat[2*4+0] * r0[0][1] + pMat[2*4+1] * r0[1][1];
	//r2[1][0] = pMat[3*4+0] * r0[0][0] + pMat[3*4+1] * r0[1][0];
	//r2[1][1] = pMat[3*4+0] * r0[0][1] + pMat[3*4+1] * r0[1][1];

	//float * out = (float *)*this;

	//// m2 = r3 * r2;
	//out[2*4+0] = r3[0][0] * r2[0][0] + r3[0][1] * r2[1][0];
	//out[2*4+1] = r3[0][0] * r2[0][1] + r3[0][1] * r2[1][1];
	//out[3*4+0] = r3[1][0] * r2[0][0] + r3[1][1] * r2[1][0];
	//out[3*4+1] = r3[1][0] * r2[0][1] + r3[1][1] * r2[1][1];

	//// m0 = r0 - r1 * m2;
	//out[0*4+0] = r0[0][0] - r1[0][0] * out[2*4+0] - r1[0][1] * out[3*4+0];
	//out[0*4+1] = r0[0][1] - r1[0][0] * out[2*4+1] - r1[0][1] * out[3*4+1];
	//out[1*4+0] = r0[1][0] - r1[1][0] * out[2*4+0] - r1[1][1] * out[3*4+0];
	//out[1*4+1] = r0[1][1] - r1[1][0] * out[2*4+1] - r1[1][1] * out[3*4+1];

	//// m1 = r1 * r3;
	//out[0*4+2] = r1[0][0] * r3[0][0] + r1[0][1] * r3[1][0];
	//out[0*4+3] = r1[0][0] * r3[0][1] + r1[0][1] * r3[1][1];
	//out[1*4+2] = r1[1][0] * r3[0][0] + r1[1][1] * r3[1][0];
	//out[1*4+3] = r1[1][0] * r3[0][1] + r1[1][1] * r3[1][1];

	//// m3 = -r3;
	//out[2*4+2] = -r3[0][0];
	//out[2*4+3] = -r3[0][1];
	//out[3*4+2] = -r3[1][0];
	//out[3*4+3] = -r3[1][1];
}

void Matrix::Translation(const Vec3 &v)
{
	::memset(m, 0, 12 * sizeof(m11));
	m11 = m22 = m33 = m44 = 1.f;
	m41 = v.x; m42 = v.y; m43 = v.z;
}

void Matrix::Scale(const Vec3 &v)
{
	::memset(m, 0, sizeof(m));
	m11 = v.x;
	m22 = v.y;
	m33 = v.z;
	m44 = 1.f;
}

void Matrix::RotationAxis(const Vec3 &axis, float angle, const Vec3 & point)
{
	float s, c, mag2;
	float xx, yy, zz, xy, yz, zx, xs, ys, zs, one_c;
	Vec3 normAxis;

	s = sinf(angle);
	c = cosf(angle);

	mag2 = axis.x * axis.x + axis.y * axis.y + axis.z * axis.z;

	/*if (mag <= 1.0e-4)
	 {
		// generate an identity matrix and return
		::memcpy(m, Identity, sizeof(float)*16);
		return;
	}*/

	if (mag2 == 1.f) {
		normAxis = axis;
	} else {
		float f = 1.f / sqrtf(mag2);
		normAxis.x = axis.x * f;
		normAxis.y = axis.y * f;
		normAxis.z = axis.z * f;
	}

	xx = normAxis.x * normAxis.x;
	yy = normAxis.y * normAxis.y;
	zz = normAxis.z * normAxis.z;
	xy = normAxis.x * normAxis.y;
	yz = normAxis.y * normAxis.z;
	zx = normAxis.z * normAxis.x;
	xs = normAxis.x * s;
	ys = normAxis.y * s;
	zs = normAxis.z * s;
	one_c = 1.f - c;

	m11 = (one_c * xx) + c;
	m12 = (one_c * xy) + zs;
	m13 = (one_c * zx) - ys;
	m14 = 0.f;

	m21 = (one_c * xy) - zs;
	m22 = (one_c * yy) + c;
	m23 = (one_c * yz) + xs;
	m24 = 0.f;

	m31 = (one_c * zx) + ys;
	m32 = (one_c * yz) - xs;
	m33 = (one_c * zz) + c;
	m34 = 0.f;

	m41 = point.x - (point.x*m11 + point.y*m21 + point.z*m31);
	m42 = point.y - (point.x*m12 + point.y*m22 + point.z*m32);
	m43 = point.z - (point.x*m13 + point.y*m23 + point.z*m33);
	m44 = 1.f;
}

void Matrix::RotationQuaternion(const Quaternion &q)
{
	m11 = 1.f - 2.f * q.y * q.y - 2.f * q.z * q.z;
	m12 = 2.f * q.x * q.y + 2.f * q.w * q.z;
	m13 = 2.f * q.x * q.z - 2.f * q.w * q.y;

	m21 = 2.f * q.x * q.y - 2.f * q.w * q.z;
	m22 = 1.f - 2.f * q.x * q.x - 2.f * q.z * q.z;
	m23 = 2.f * q.y * q.z + 2.f * q.w * q.x;

	m31 = 2.f * q.x * q.z + 2.f * q.w * q.y;
	m32 = 2.f * q.y * q.z - 2.f * q.w * q.x;
	m33 = 1.f - 2.f * q.x * q.x - 2.f * q.y * q.y;

	m14 = m24 = m34 = m41 = m42 = m43 = 0.f;
	m44 = 1.f;
}

void Matrix::RotationYawPitchRoll(float yaw, float pitch, float roll)
{// http://en.wikipedia.org/wiki/Euler_angles
	float s1 = sinf(roll);
	float c1 = cosf(roll);
	float s2 = sinf(pitch);
	float c2 = cosf(pitch);
	float s3 = sinf(-yaw);
	float c3 = cosf(yaw);

	// X1 Y2 Z3

	m11 = c2 * c3;
	m12 = -c2 * s3;
	m13 = s2;

	m21 = c1 * s3 + c3 * s1 * s2;
	m22 = c1 * c3 - s1 * s2 * s3;
	m23 = -c2 * s1;

	m31 = s1 * s3 - c1 * c3 * s2;
	m32 = c3 * s1 + c1 * s2 * s3;
	m33 = c1 * c2;

	m14 = m24 = m34 = m41 = m42 = m43 = 0.f;
	m44 = 1.f;
}

//void Matrix::Reflect(const sPlane &plane)
//{
//	float f, mag2, dist;
//	Vec3 normal;
//
//	mag2 = plane.normal.x * plane.normal.x + plane.normal.y * plane.normal.y + plane.normal.z * plane.normal.z;
//	if (mag2 == 1.0) {
//		normal = plane.normal;
//		dist = plane.dist;
//	} else {
//		f = 1.f / sqrtf(mag2);
//		normal.x = plane.normal.x * f;
//		normal.y = plane.normal.y * f;
//		normal.z = plane.normal.z * f;
//		dist = plane.dist * f;
//	}
//
//	f = 2 * normal.x;
//	m11 = 1.f - f * normal.x;
//	m21 = -f * normal.y;
//	m31 = -f * normal.z;
//	m41 = f * dist;
//
//	f = 2 * normal.y;
//	m12 = -f * normal.x;
//	m22 = 1.f - f * normal.y;
//	m32 = -f * normal.z;
//	m42 = f * dist;
//
//	f = 2 * normal.z;
//	m13 = -f * normal.x;
//	m23 = -f * normal.y;
//	m33 = 1.f - f * normal.z;
//	m43 = f * dist;
//
//	m14 = m24 = m34 = 0.f;
//	m44 = 1.f;
//}

void Matrix::Transformation(const Vec3& vSC,
							const Quaternion& qSR,
							const Vec3& vS,
							const Vec3& vRC,
							const Quaternion& qR,
							const Vec3& vT)
{
	Matrix Msc, MscInv, Msr, MsrInv, Ms, Mrc, MrcInv, Mr, Mt;

	Vec3 vSCInv (-vSC.x, -vSC.y, -vSC.z);
	Vec3 vRCInv (-vRC.x, -vRC.y, -vRC.z);
	Quaternion qSRInv (qSR.x, qSR.y, qSR.z, -qSR.w);

	Msc.Translation(vSC);
	MscInv.Translation(vSCInv);
	Msr.RotationQuaternion(qSR);
	MsrInv.RotationQuaternion(qSRInv);
	Ms.SetIdentity(); Ms.m11 = vS.x; Ms.m22 = vS.y; Ms.m33 = vS.z;
	Mrc.Translation(vRC);
	MrcInv.Translation(vRCInv);
	Mr.RotationQuaternion(qR);
	Mt.Translation(vT);

	*this = MscInv * MsrInv * Ms * Msr * Msc * MrcInv * Mr * Mrc * Mt;
}

Vec3 Matrix::operator * (const Vec3 &ref) const
{
	return Vec3(m11 * ref.x + m12 * ref.y + m13 * ref.z,
				m21 * ref.x + m22 * ref.y + m23 * ref.z,
				m31 * ref.x + m32 * ref.y + m33 * ref.z);
}

void Matrix::Orthonormalize()
{
	float fLength2 = (m11 * m11) + (m21 * m21) + (m31 * m31);
	if (fLength2 > 0.f)
	{
		float f = 1.f / sqrtf(fLength2);
		m11 *= f;
		m21 *= f;
		m31 *= f;
	}

	{
		float f = m11 * m12 + m21 * m22 + m31 * m32;
		m12 -= m11 * f;
		m22 -= m21 * f;
		m32 -= m31 * f;
	}

	fLength2 = (m12 * m12) + (m22 * m22) + (m32 * m32);
	if (fLength2 > 0.f)
	{
		float f = 1.f / sqrtf(fLength2);
		m12 *= f;
		m22 *= f;
		m32 *= f;
	}

	m13 = (m21 * m32) - (m31 * m22);
	m23 = (m31 * m12) - (m11 * m32);
	m33 = (m11 * m22) - (m21 * m12);

	//Vec3 axis_x(m11, m21, m31);
	//axis_x.Normalize();
	//m11 = axis_x.x; m21 = axis_x.y; m31 = axis_x.z;
	//Vec3 axis_y(m12, m22, m32);
	//axis_y -= axis_x * Vec3::Dot(axis_x, axis_y);
	//axis_y.Normalize();
	//m12 = axis_y.x; m22 = axis_y.y; m32 = axis_y.z;
}

void mr::MatrixMultiply(Matrix & out, const Matrix & a, const Matrix & b)
{
	Matrix mat;
	Matrix & target = (&out != &a && &out != &b) ? out : mat;
	//Matrix & target = out;
	
	target.m11 = a.m11*b.m11 + a.m12*b.m21 + a.m13*b.m31 + a.m14*b.m41;
	target.m12 = a.m11*b.m12 + a.m12*b.m22 + a.m13*b.m32 + a.m14*b.m42;
	target.m13 = a.m11*b.m13 + a.m12*b.m23 + a.m13*b.m33 + a.m14*b.m43;
	target.m14 = a.m11*b.m14 + a.m12*b.m24 + a.m13*b.m34 + a.m14*b.m44;
	
	target.m21 = a.m21*b.m11 + a.m22*b.m21 + a.m23*b.m31 + a.m24*b.m41;
	target.m22 = a.m21*b.m12 + a.m22*b.m22 + a.m23*b.m32 + a.m24*b.m42;
	target.m23 = a.m21*b.m13 + a.m22*b.m23 + a.m23*b.m33 + a.m24*b.m43;
	target.m24 = a.m21*b.m14 + a.m22*b.m24 + a.m23*b.m34 + a.m24*b.m44;
	
	target.m31 = a.m31*b.m11 + a.m32*b.m21 + a.m33*b.m31 + a.m34*b.m41;
	target.m32 = a.m31*b.m12 + a.m32*b.m22 + a.m33*b.m32 + a.m34*b.m42;
	target.m33 = a.m31*b.m13 + a.m32*b.m23 + a.m33*b.m33 + a.m34*b.m43;
	target.m34 = a.m31*b.m14 + a.m32*b.m24 + a.m33*b.m34 + a.m34*b.m44;
	
	target.m41 = a.m41*b.m11 + a.m42*b.m21 + a.m43*b.m31 + a.m44*b.m41;
	target.m42 = a.m41*b.m12 + a.m42*b.m22 + a.m43*b.m32 + a.m44*b.m42;
	target.m43 = a.m41*b.m13 + a.m42*b.m23 + a.m43*b.m33 + a.m44*b.m43;
	target.m44 = a.m41*b.m14 + a.m42*b.m24 + a.m43*b.m34 + a.m44*b.m44;
	
	if (&out != &target) out = target;
}

Vec3 Matrix::Rotation() const
{
	const float MIN_ANGLE_VALUE = 0.00001f;
	Vec3 dirX = Vec3::Normalize(AxisX());
	Vec3 vAngles;
	if (dirX.z > 0.9999f)
	{
		vAngles.y = 90.f;
		vAngles.x = 0.f;
		vAngles.z = -RAD2DEG(atan2f(m21, m22));
	}
	else if (dirX.z < -0.9999f)
	{
		vAngles.y = -90.f;
		vAngles.x = 0.f;
		vAngles.z = -RAD2DEG(atan2f(m21, m22));
	}
	else
	{
		vAngles.z = RAD2DEG(atan2f(dirX.y, dirX.x));
		vAngles.y = RAD2DEG(asinf(dirX.z));
		vAngles.x = RAD2DEG(atan2f(-m23 / AxisY().Length(), m33 / AxisZ().Length()));
		if (fabsf(vAngles.x) < MIN_ANGLE_VALUE) vAngles.x = 0.f;
		if (fabsf(vAngles.y) < MIN_ANGLE_VALUE) vAngles.y = 0.f;
	}
	if (fabsf(vAngles.z) < MIN_ANGLE_VALUE) vAngles.z = 0.f;

	return vAngles;
}

void Matrix::SetFrustum(float left, float right, float bottom, float top, float znear, float zfar)
{
	m11 = 2.f * znear / (right - left);
	m12 = 0.f;
	m13 = 0.f;
	m14 = 0.f;

	m21 = 0.f;
	m22 = 2.f * znear / (top - bottom);
	m23 = 0.f;
	m24 = 0.f;

	m31 = (right + left) / (right - left);
	m32 = (top + bottom) / (top - bottom);
	m33 = -(zfar + znear) / (zfar - znear);
	m34 = -1.f;

	m41 = 0.f;
	m42 = 0.f;
	m43 = (-2.f * zfar * znear) / (zfar - znear);
	m44 = 0.f;
}

