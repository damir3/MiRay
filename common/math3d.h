#pragma once

namespace mr
{

class MatrixBuilder
{
	Matrix mat;
public:

	MatrixBuilder() { mat.SetIdentity(); }

	operator const Matrix & () const { return mat; }

	MatrixBuilder & AddPosition(const Vec3 & pos);
	MatrixBuilder & AddRotation(const Vec3 & ang);
	MatrixBuilder & AddScale(const Vec3 & scale);
};

template<typename T>
struct TRect
{
	T	left, top;
	T	right, bottom;

	TRect() {}
	TRect(const TRect & rect) : left(rect.left), top(rect.top), right(rect.right), bottom(rect.bottom) {}
	TRect(T x1, T y1, T x2, T y2) : left(x1), top(y1), right(x2), bottom(y2) {}

	T Width() const {return right - left;}
	T Height() const {return bottom - top;}
};

typedef TRect<int>		RectI;
typedef TRect<float>	RectF;

struct Vertex
{
	Vec3	pos;
	Vec3	normal;
	Vec2	tc;

	Vertex() {}
	Vertex(const Vec3 & p) : pos(p) {}
	Vertex(const Vec3 & p, const Vec3 & n, const Vec2 & t) : pos(p), normal(n), tc(t) {}
};

Vec2 Vec2Rand();
Vec3 Vec3Rand();

Vec3 VectorToAngles(const Vec3 &vDir);
Vec3 AnglesToVector(const Vec3 &vAngles);

Vec3 Vec3Hermite(float t, const Vec3 & p0, const Vec3 & m0, const Vec3 & p1, const Vec3 & m1);

float CatmullRom(float t, const float f[4]);
float CatmullRom2(float s, float t, const float f[16]);

void ComputeTangents(Vec3 & tangent, Vec3 & binormal, Vertex &v0, const Vertex & v1, const Vertex & v2);

void MatrixPerspectiveFov(Matrix & mat, float fovY, float aspect, float zNear, float zFar);
void MatrixLookAt(Matrix & mat, const Vec3 & vEye, const Vec3 & vAt, const Vec3 & vUp);
void CubemapViewMatrix(Matrix & mat, const Vec3 & vEye, int view);

void CalculateProjectionMatrix(Matrix & matProjection, float fFOV, float fAspect, float zNear = 0.1f, float zFar = 1000.f);
void CalculateCameraMatrix(Matrix & matCamera, const Vec3 & vCenter, float fDistance, float fYaw, float fPitch);

float VertexLight(const Vec3 & vNormal);
Color VertexColor(const Color & c, const Vec3 & vNormal);

}