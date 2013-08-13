#include "math3d.h"

using namespace mr;

// ------------------------------------------------------------------------ //
// Matrix math
// ------------------------------------------------------------------------ //

MatrixBuilder & MatrixBuilder::AddPosition(const Vec3 & pos)
{
	mat.m41 = pos.x;
	mat.m42 = pos.y;
	mat.m43 = pos.z;

	return *this;
}

MatrixBuilder & MatrixBuilder::AddRotation(const Vec3 & ang)
{// http://en.wikipedia.org/wiki/Euler_angles
	float roll = DEG2RAD(ang.x);
	float pitch = DEG2RAD(ang.y);
	float yaw = DEG2RAD(ang.z);

	float s1 = sinf(roll);
	float c1 = cosf(roll);
	float s2 = sinf(pitch);
	float c2 = cosf(pitch);
	float s3 = sinf(-yaw);
	float c3 = cosf(yaw);

	// X1 Y2 Z3

	mat.m11 = c2 * c3;
	mat.m12 = -c2 * s3;
	mat.m13 = s2;

	mat.m21 = c1 * s3 + c3 * s1 * s2;
	mat.m22 = c1 * c3 - s1 * s2 * s3;
	mat.m23 = -c2 * s1;

	mat.m31 = s1 * s3 - c1 * c3 * s2;
	mat.m32 = c3 * s1 + c1 * s2 * s3;
	mat.m33 = c1 * c2;

	return *this;
}

MatrixBuilder & MatrixBuilder::AddScale(const Vec3 & scale)
{
	mat.m11 *= scale.x;
	mat.m12 *= scale.x;
	mat.m13 *= scale.x;
	mat.m21 *= scale.y;
	mat.m22 *= scale.y;
	mat.m23 *= scale.y;
	mat.m31 *= scale.z;
	mat.m32 *= scale.z;
	mat.m33 *= scale.z;

	return *this;
}

// ------------------------------------------------------------------------ //

Vec2 mr::Vec2Rand()
{
	Vec2 out;
	do 
	{
		out.x = rand() * (2.f / RAND_MAX) - 1.f;
		out.y = rand() * (2.f / RAND_MAX) - 1.f;
	} while (out.LengthSquared() > 1.f);
	return out;
}

Vec3 mr::Vec3Rand()
{
	Vec3 out;
	do 
	{
		out.x = rand() * (2.f / RAND_MAX) - 1.f;
		out.y = rand() * (2.f / RAND_MAX) - 1.f;
		out.z = rand() * (2.f / RAND_MAX) - 1.f;
	} while (out.LengthSquared() > 1.f);
	return out;
}

// ------------------------------------------------------------------------ //
// Vector math
// ------------------------------------------------------------------------ //

Vec3 mr::VectorToAngles(const Vec3 &vDir)
{
	return Vec3(0.f, RAD2DEG(vDir.Pitch()), RAD2DEG(vDir.Yaw()));
}

Vec3 mr::AnglesToVector(const Vec3 &vAngles)
{
	float cx = cosf(DEG2RAD(vAngles.y));
	float sx = sinf(DEG2RAD(vAngles.y));
	float cz = cosf(DEG2RAD(vAngles.z));
	float sz = sinf(DEG2RAD(vAngles.z));
	return Vec3(cz*cx, sz*cx, sx);
}

// ------------------------------------------------------------------------ //
// Hermite spline math
// ------------------------------------------------------------------------ //

Vec3 mr::Vec3Hermite(float t, const Vec3 & p0, const Vec3 & m0, const Vec3 & p1, const Vec3 & m1)
{
	float t2 = t * t, t3 = t2 * t;
	return p0 * (2 * t3 - 3 * t2 + 1) + (t3 - 2 * t2 + t) * m0 + (-2 * t3 + 3 * t2) * p1 + (t3 - t2) * m1;
}

//Vec3 mr::Vec3HermiteDerivative(float t, const Vec3 & p0, const Vec3 & m0, const Vec3 & p1, const Vec3 & m1)
//{
//	float t2 = t * t;
//	return p0 * (6 * t2 - 6 * t) + (3 * t2 - 4 * t + 1) * m0 + (-6 * t2 + 6 * t) * p1 + (3 * t2 - 2 * t) * m1;
//}


// ------------------------------------------------------------------------ //
// CatmullRom
// ------------------------------------------------------------------------ //
//
// q(t) = 0.5 * [ t^3 t^2 t 1 ] * [ -1  3 -3  1 ] * [ p1 ]
//                                [  2 -5  4 -1 ]   [ p2 ]
//                                [ -1  0  1  0 ]   [ p3 ]
//                                [  0  2  0  0 ]   [ p4 ]
//
// q(t) = 0.5 * ((-p1 + 3*p2 -3*p3 + p4)*t*t*t
//             + (2*p1 -5*p2 + 4*p3 - p4)*t*t
//             + (-p1+p3)*t
//             + 2*p2)
//
// ------------------------------------------------------------------------ //

float mr::CatmullRom(float t, const float f[4])
{
	float t2 = t*t, t3 = t2*t;
	return 0.5f * ((-t*(1-t)*(1-t))*f[0] + (2-5*t2+3*t3)*f[1] + (t+4*t2-3*t3)*f[2] - (t2*(1-t))*f[3]);
}

float mr::CatmullRom2(float s, float t, const float f[16])
{
	float tmp[4];
	tmp[0] = CatmullRom(s, f + 0);
	tmp[1] = CatmullRom(s, f + 4);
	tmp[2] = CatmullRom(s, f + 8);
	tmp[3] = CatmullRom(s, f + 12);
	return CatmullRom(t, tmp);
}

// ------------------------------------------------------------------------ //

void mr::ComputeTangents(Vec3 & tangent, Vec3 & binormal, Vertex &v0, const Vertex & v1, const Vertex & v2)
{
	const Vec3 dP1 = v0.pos - v1.pos;
	const Vec3 dP2 = v0.pos - v2.pos;
	const Vec2 dUV1 = v0.tc - v1.tc;
	const Vec2 dUV2 = v0.tc - v2.tc;

	float f = (dUV1.x * dUV2.y - dUV1.y * dUV2.x);
	if (f != 0.f) {
		//f = 1.f / f; // faster if comment
		tangent = Vec3((dUV2.y * dP1.x - dUV1.y * dP2.x), (dUV2.y * dP1.y - dUV1.y * dP2.y), (dUV2.y * dP1.z - dUV1.y * dP2.z)) * f;
		binormal = Vec3((dUV1.x * dP2.x - dUV2.x * dP1.x), (dUV1.x * dP2.y - dUV2.x * dP1.y), (dUV1.x * dP2.z - dUV2.x * dP1.z)) * f;
	} else {
		tangent = Vec3::X;
		binormal = Vec3::Y;
	}

	tangent -= v0.normal * Vec3::Dot(tangent, v0.normal);
	tangent.Normalize();
	binormal -= v0.normal * Vec3::Dot(binormal, v0.normal);
	binormal.Normalize();
}

// ------------------------------------------------------------------------ //

void mr::MatrixPerspectiveFov(Matrix & mat, float fovY, float aspect, float zNear, float zFar)
{
	float yScale = 1.f / tanf(DEG2RAD(fovY) * 0.5f);
	float xScale = yScale / aspect;
	mat = Matrix(xScale, 0.f, 0.f, 0.f,
				 0.f, yScale, 0.f, 0.f,
				 0.f, 0.f, (zFar + zNear) / (zFar - zNear), 1.f,
				 0.f, 0.f, 2.f * zFar * zNear / (zNear - zFar), 0.f);
}

void mr::MatrixLookAt(Matrix & mat, const Vec3 & vEye, const Vec3 & vAt, const Vec3 & vUp)
{
	Vec3 zAxis = Vec3::Normalize(vAt - vEye);
	Vec3 xAxis = Vec3::Normalize(Vec3::Cross(zAxis, vUp));
	Vec3 yAxis = Vec3::Cross(xAxis, zAxis);
	mat = Matrix(xAxis.x, yAxis.x, zAxis.x, 0.f,
				 xAxis.y, yAxis.y, zAxis.y, 0.f,
				 xAxis.z, yAxis.z, zAxis.z, 0.f,
				 -Vec3::Dot(xAxis, vEye), -Vec3::Dot(yAxis, vEye), -Vec3::Dot(zAxis, vEye), 1.f);
}

void mr::CubemapViewMatrix(Matrix & mat, const Vec3 & vEye, int view)
{
	Vec3 xAxis;
	Vec3 yAxis;
	Vec3 zAxis;
	switch (view)
	{
	case 0:
		xAxis = -Vec3::Z;
		yAxis = -Vec3::Y;
		zAxis = Vec3::X;
		break;
	case 1:
		xAxis = Vec3::Z;
		yAxis = -Vec3::Y;
		zAxis = -Vec3::X;
		break;
	case 2:
		xAxis = Vec3::X;
		yAxis = Vec3::Z;
		zAxis = Vec3::Y;
		break;
	case 3:
		xAxis = Vec3::X;
		yAxis = -Vec3::Z;
		zAxis = -Vec3::Y;
		break;
	case 4:
		xAxis = Vec3::X;
		yAxis = -Vec3::Y;
		zAxis = Vec3::Z;
		break;
	case 5:
		xAxis = -Vec3::X;
		yAxis = -Vec3::Y;
		zAxis = -Vec3::Z;
		break;
	}
	mat = Matrix(xAxis.x, yAxis.x, zAxis.x, 0.f,
				 xAxis.y, yAxis.y, zAxis.y, 0.f,
				 xAxis.z, yAxis.z, zAxis.z, 0.f,
				 -Vec3::Dot(xAxis, vEye), -Vec3::Dot(yAxis, vEye), -Vec3::Dot(zAxis, vEye), 1.f);
}

// ------------------------------------------------------------------------ //

void mr::CalculateProjectionMatrix(Matrix & matProjection, float fFOV, float fAspect, float zNear, float zFar)
{
	float dy = zNear * tanf( DEG2RAD(fFOV)*0.5f );
	float dx = dy * fAspect;
	matProjection.SetFrustum(-dx, dx, -dy, dy, zNear, zFar);
}

void mr::CalculateCameraMatrix(Matrix & matCamera, const Vec3 & vCenter, float fDistance, float fYaw, float fPitch)
{
	Vec3 vAngles(0.f, fPitch, fYaw);
	matCamera = MatrixBuilder().AddPosition(AnglesToVector(vAngles) * fDistance + vCenter).AddRotation(vAngles);
	Matrix matCameraSpace(0.f, 1.f, 0.f, 0.f,
						  0.f, 0.f, 1.f, 0.f,
						  1.f, 0.f, 0.f, 0.f,
						  0.f, 0.f, 0.f, 1.f);
	matCamera = matCameraSpace * matCamera;
}

// ------------------------------------------------------------------------ //

float mr::VertexLight(const Vec3 & vNormal)
{
	static const Vec3 vLightDir = Vec3::Normalize(Vec3(1.f, -2.f, 3.f));
	return Vec3::Dot(vNormal, vLightDir) * 0.35f + 0.65f;
}

Color mr::VertexColor(const Color & c, const Vec3 & vNormal)
{
	float f = VertexLight(vNormal);
	Color result;
	result.r = byte(c.r * f);
	result.g = byte(c.g * f);
	result.b = byte(c.b * f);
	result.a = c.a;
	return result;
}


