#pragma once

#include <algorithm>
#include <glm/glm.hpp>

#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective
#include <glm/gtc/quaternion.hpp> // glm::quat
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp> // glm::length2, glm::distance2
#include <glm/gtx/rotate_vector.hpp>
#undef GLM_ENABLE_EXPERIMENTAL

#ifdef GLM_USE_SHORT_TYPES
using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using mat4 = glm::mat4;
using quat = glm::quat;
using ivec2 = glm::ivec2;
using dvec2 = glm::dvec2;
using dvec3 = glm::dvec3;
#endif

using byte = uint8_t;

#ifndef M_PI
#define M_PI            glm::pi<double>()
#endif

#define M_PIf           glm::pi<float>()
#define M_2PIf          glm::two_pi<float>()
#define M_HALF_PIf      glm::half_pi<float>()

#define DELTA_EPSILON   0.0001f

#define F2B(a)          static_cast<uint8_t>((a)*255.f)
#define B2F(a)          ((a)*(1.f/255.f))
#define SF2B(a)         static_cast<byte>(std::clamp((a), 0.f, 1.f) * 255.f + .5f)

template<typename T>
inline T lerp(const T &a, const T &b, float f)
{
	return T(a + (b - a) * f);
}

template <typename T>
inline T lerp2(T v0, T v1, T v2, T v3, float s, float t)
{
	float vt0 = lerp(float(v0), float(v2), t);
	float vt1 = lerp(float(v1), float(v3), t);
	return T(lerp(vt0, vt1, s));
}

template<class T>
inline T sqr(T value)
{
	return value * value;
}

namespace glm
{
	GLM_FUNC_QUALIFIER glm::vec3 transformCoord(const glm::vec3 & tmp, const glm::mat4 &mat)
	{
		return glm::vec3(tmp.x*mat[0][0] + tmp.y*mat[1][0] + tmp.z*mat[2][0] + mat[3][0],
						 tmp.x*mat[0][1] + tmp.y*mat[1][1] + tmp.z*mat[2][1] + mat[3][1],
						 tmp.x*mat[0][2] + tmp.y*mat[1][2] + tmp.z*mat[2][2] + mat[3][2]);
	}

	GLM_FUNC_QUALIFIER glm::vec3 transformNormal(const glm::vec3 & tmp, const glm::mat4 &mat)
	{
		return glm::vec3(tmp.x*mat[0][0] + tmp.y*mat[1][0] + tmp.z*mat[2][0],
						 tmp.x*mat[0][1] + tmp.y*mat[1][1] + tmp.z*mat[2][1],
						 tmp.x*mat[0][2] + tmp.y*mat[1][2] + tmp.z*mat[2][2]);
	}

	GLM_FUNC_QUALIFIER glm::vec3 ttransformNormal(const glm::vec3 & tmp, const glm::mat4 &mat)
	{
		return glm::vec3(tmp.x*mat[0][0] + tmp.y*mat[0][1] + tmp.z*mat[0][2],
						 tmp.x*mat[1][0] + tmp.y*mat[1][1] + tmp.z*mat[1][2],
						 tmp.x*mat[2][0] + tmp.y*mat[2][1] + tmp.z*mat[2][2]);
	}

	GLM_FUNC_QUALIFIER glm::vec3 ttransformCoord(const glm::vec3 & tmp, const glm::mat4 &mat)
	{
		return glm::vec3(tmp.x*mat[0][0] + tmp.y*mat[0][1] + tmp.z*mat[0][2] + mat[0][3],
						 tmp.x*mat[1][0] + tmp.y*mat[1][1] + tmp.z*mat[1][2] + mat[1][3],
						 tmp.x*mat[2][0] + tmp.y*mat[2][1] + tmp.z*mat[2][2] + mat[2][3]);
	}

	GLM_FUNC_QUALIFIER glm::vec3 perpendicular(const glm::vec3 & v)
	{
		glm::vec3 dir;
		if (std::fabs(v.x) <= std::fabs(v.y) && std::fabs(v.x) <= std::fabs(v.z))
			dir = glm::vec3(v.y*v.y+v.z*v.z, -v.x*v.y, -v.x*v.z);
		else if (std::fabs(v.y) <= std::fabs(v.z))
			dir = glm::vec3(-v.y*v.x, v.x*v.x+v.z*v.z, -v.y*v.z);
		else
			dir = glm::vec3(-v.z*v.x, -v.z*v.y, v.x*v.x+v.y*v.y);

		return glm::normalize(dir);
	}

	GLM_FUNC_QUALIFIER glm::mat4 rotationYawPitchRoll(float yaw, float pitch, float roll)
	{// http://en.wikipedia.org/wiki/Euler_angles
		float s1 = sinf(roll);
		float c1 = cosf(roll);
		float s2 = sinf(pitch);
		float c2 = cosf(pitch);
		float s3 = sinf(-yaw);
		float c3 = cosf(yaw);

		// X1 Y2 Z3

		glm::mat4 m;
		m[0][0] = c2 * c3;
		m[0][1] = -c2 * s3;
		m[0][2] = s2;

		m[1][0] = c1 * s3 + c3 * s1 * s2;
		m[1][1] = c1 * c3 - s1 * s2 * s3;
		m[1][2] = -c2 * s1;

		m[2][0] = s1 * s3 - c1 * c3 * s2;
		m[2][1] = c3 * s1 + c1 * s2 * s3;
		m[2][2] = c1 * c2;

		m[0][3] = m[1][3] = m[2][3] = m[3][0] = m[3][1] = m[3][2] = 0.f;
		m[3][3] = 1.f;

		return m;
	}

	GLM_FUNC_QUALIFIER const glm::vec3 & axis(const glm::mat4 & m, int i) { return *(const glm::vec3 *)&m[i]; }
	GLM_FUNC_QUALIFIER const glm::vec3 & axisX(const glm::mat4 & m) { return axis(m, 0); }
	GLM_FUNC_QUALIFIER const glm::vec3 & axisY(const glm::mat4 & m) { return axis(m, 1); }
	GLM_FUNC_QUALIFIER const glm::vec3 & axisZ(const glm::mat4 & m) { return axis(m, 2); }
	GLM_FUNC_QUALIFIER const glm::vec3 & translation(const glm::mat4 & m) { return axis(m, 3); }

	GLM_FUNC_QUALIFIER void setAxis(glm::mat4 & m, int i, const glm::vec3 & v) { *(glm::vec3 *)&m[i] = v; }
	GLM_FUNC_QUALIFIER void setAxisX(glm::mat4 & m, const glm::vec3 & v) { setAxis(m, 0, v); }
	GLM_FUNC_QUALIFIER void setAxisY(glm::mat4 & m, const glm::vec3 & v) { setAxis(m, 1, v); }
	GLM_FUNC_QUALIFIER void setAxisZ(glm::mat4 & m, const glm::vec3 & v) { setAxis(m, 2, v); }
	GLM_FUNC_QUALIFIER void setTranslation(glm::mat4 & m, const glm::vec3 & v) { setAxis(m, 3, v); }

	GLM_FUNC_QUALIFIER glm::vec3 getRotation(const glm::mat4 & m)
	{
		const float MIN_ANGLE_VALUE = 0.00001f;
		const glm::vec3 & mAxisY = axisY(m);
		glm::vec3 dirX = glm::normalize(axisX(m));
		glm::vec3 angles;
		if (dirX.z > 0.9999f)
		{
			angles.y = 90.f;
			angles.x = 0.f;
			angles.z = -glm::degrees(std::atan2(mAxisY.x, mAxisY.y));
		}
		else if (dirX.z < -0.9999f)
		{
			angles.y = -90.f;
			angles.x = 0.f;
			angles.z = -glm::degrees(std::atan2(mAxisY.x, mAxisY.y));
		}
		else
		{
			const auto & mAxisZ = axisZ(m);
			angles.z = glm::degrees(std::atan2(dirX.y, dirX.x));
			angles.y = glm::degrees(std::asin(dirX.z));
			angles.x = glm::degrees(std::atan2(-mAxisY.z / glm::length(mAxisY), mAxisZ.z / glm::length(mAxisZ)));
			if (std::fabs(angles.x) < MIN_ANGLE_VALUE) angles.x = 0.f;
			if (std::fabs(angles.y) < MIN_ANGLE_VALUE) angles.y = 0.f;
		}
		if (std::fabs(angles.z) < MIN_ANGLE_VALUE) angles.z = 0.f;

		return angles;
	}

	GLM_FUNC_QUALIFIER glm::vec3 getScale(const glm::mat4 & m)
	{
		return glm::vec3(glm::length(axisX(m)), glm::length(axisY(m)), glm::length(axisZ(m)));
	}
}

struct BBox {
	glm::vec3 min;
	glm::vec3 max;

	BBox() {}
	BBox(const BBox & box) : min(box.min), max(box.max) {}
	BBox(const glm::vec3 & min_, const glm::vec3 & max_) : min(min_), max(max_) {}

	void clear()
	{
		min.x = min.y = min.z = FLT_MAX;
		max.x = max.y = max.z = -FLT_MAX;
	}

	void addToBounds(const glm::vec3 & v)
	{
		if (min.x > v.x) min.x = v.x;
		if (max.x < v.x) max.x = v.x;
		if (min.y > v.y) min.y = v.y;
		if (max.y < v.y) max.y = v.y;
		if (min.z > v.z) min.z = v.z;
		if (max.z < v.z) max.z = v.z;
	}

	void addToBounds(const BBox & box)
	{
		if (min.x > box.min.x) min.x = box.min.x;
		if (max.x < box.max.x) max.x = box.max.x;
		if (min.y > box.min.y) min.y = box.min.y;
		if (max.y < box.max.y) max.y = box.max.y;
		if (min.z > box.min.z) min.z = box.min.z;
		if (max.z < box.max.z) max.z = box.max.z;
	}

	bool isNull() const
	{
		return min.x > max.x || min.y > max.y || min.z > max.z;
	}

	bool isEmpty() const
	{
		return min.x >= max.x || min.y >= max.y || min.z >= max.z;
	}

	glm::vec3 center() const
	{
		return (min + max) * 0.5f;
	}

	glm::vec3 size() const
	{
		if (min.x > max.x || min.y > max.y || min.z > max.z)
			return glm::vec3(0.f);

		return max - min;
	}

	void getVerts(glm::vec3 out[8]) const
	{
		out[0].x = out[2].x = out[4].x = out[6].x = min.x;
		out[1].x = out[3].x = out[5].x = out[7].x = max.x;
		out[0].y = out[1].y = out[4].y = out[5].y = min.y;
		out[2].y = out[3].y = out[6].y = out[7].y = max.y;
		out[0].z = out[1].z = out[2].z = out[3].z = min.z;
		out[4].z = out[5].z = out[6].z = out[7].z = max.z;
	}

	bool intersectsBox(const BBox & box) const
	{
		if (box.max.x < min.x) return false;
		if (box.min.x > max.x) return false;
		if (box.max.y < min.y) return false;
		if (box.min.y > max.y) return false;
		if (box.max.z < min.z) return false;
		if (box.min.z > max.z) return false;
		return true;
	}

	bool containsPoint(const glm::vec3 & v) const
	{
		if (v.x < min.x) return false;
		if (v.x > max.x) return false;
		if (v.y < min.y) return false;
		if (v.y > max.y) return false;
		if (v.z < min.z) return false;
		if (v.z > max.z) return false;
		return true;
	}

	void transform(const glm::mat4 & mat)
	{
		auto pos = center();
		auto ext = pos - min;
		pos = glm::transformCoord(pos, mat);
		glm::vec3 newExt(std::fabs(mat[0][0] * ext.x) + std::fabs(mat[1][0] * ext.y) + std::fabs(mat[2][0] * ext.z),
						 std::fabs(mat[0][1] * ext.x) + std::fabs(mat[1][1] * ext.y) + std::fabs(mat[2][1] * ext.z),
						 std::fabs(mat[0][2] * ext.x) + std::fabs(mat[1][2] * ext.y) + std::fabs(mat[2][2] * ext.z));
		min = pos - newExt;
		max = pos + newExt;
	}

	float distanceToPointSqr(const glm::vec3 & point) const
	{
		float sqrDist = 0;
		for (int i = 0; i < 3; i++)
		{
			if (point[i] < min[i])
				sqrDist += sqr(min[i] - point[i]);
			else if (point[i] > max[i])
				sqrDist += sqr(point[i] - max[i]);
		}
		return sqrDist;
	}

	bool intersect(const glm::vec3 & rayOrigin, const glm::vec3 & rayInvDirection, float & tmin, float & tmax)
	{
		tmin = ((rayInvDirection.x < 0.f ? max.x : min.x) - rayOrigin.x) * rayInvDirection.x;
		tmax = ((rayInvDirection.x < 0.f ? min.x : max.x) - rayOrigin.x) * rayInvDirection.x;

		float tymin = ((rayInvDirection.y < 0.f ? max.y : min.y) - rayOrigin.y) * rayInvDirection.y;
		float tymax = ((rayInvDirection.y < 0.f ? min.y : max.y) - rayOrigin.y) * rayInvDirection.y;
		if ( (tmin > tymax) || (tymin > tmax) )
			return false;

		if (tymin > tmin)
			tmin = tymin;

		if (tymax < tmax)
			tmax = tymax;

		float tzmin = ((rayInvDirection.z < 0.f ? max.z : min.z) - rayOrigin.z) * rayInvDirection.z;
		float tzmax = ((rayInvDirection.z < 0.f ? min.z : max.z) - rayOrigin.z) * rayInvDirection.z;

		if ( (tmin > tzmax) || (tzmin > tmax) )
			return false;

		if (tzmin > tmin)
			tmin = tzmin;

		if (tzmax < tmax)
			tmax = tzmax;

		return tmin < tmax;
	}

	bool operator == (const BBox &ref) const
	{
		return min == ref.min && max == ref.max;
	}

	bool operator != (const BBox &ref) const
	{
		return min != ref.min || max != ref.max;
	}
};

class MatrixBuilder
{
	glm::mat4 mat;
public:

	MatrixBuilder() : mat(1.f) {}

	operator const glm::mat4 & () const { return mat; }

	MatrixBuilder & AddPosition(const glm::vec3 & pos)
	{
		mat[3][0] = pos.x;
		mat[3][1] = pos.y;
		mat[3][2] = pos.z;

		return *this;
	}

	MatrixBuilder & AddRotation(const glm::vec3 & ang)
	{// http://en.wikipedia.org/wiki/Euler_angles
		float roll = glm::radians(ang.x);
		float pitch = glm::radians(ang.y);
		float yaw = glm::radians(ang.z);

		float s1 = sinf(roll);
		float c1 = cosf(roll);
		float s2 = sinf(pitch);
		float c2 = cosf(pitch);
		float s3 = sinf(-yaw);
		float c3 = cosf(yaw);

		// X1 Y2 Z3

		mat[0][0] = c2 * c3;
		mat[0][1] = -c2 * s3;
		mat[0][2] = s2;

		mat[1][0] = c1 * s3 + c3 * s1 * s2;
		mat[1][1] = c1 * c3 - s1 * s2 * s3;
		mat[1][2] = -c2 * s1;

		mat[2][0] = s1 * s3 - c1 * c3 * s2;
		mat[2][1] = c3 * s1 + c1 * s2 * s3;
		mat[2][2] = c1 * c2;

		return *this;
	}

	MatrixBuilder & AddScale(const glm::vec3 & scale)
	{
		mat[0][0] *= scale.x;
		mat[0][1] *= scale.x;
		mat[0][2] *= scale.x;
		mat[1][0] *= scale.y;
		mat[1][1] *= scale.y;
		mat[1][2] *= scale.y;
		mat[2][0] *= scale.z;
		mat[2][1] *= scale.z;
		mat[2][2] *= scale.z;

		return *this;
	}
};

template<typename T>
struct TRect {
	T	left, top;
	T	right, bottom;

	TRect() {}
	TRect(const TRect & rect) : left(rect.left), top(rect.top), right(rect.right), bottom(rect.bottom) {}
	TRect(T x1, T y1, T x2, T y2) : left(x1), top(y1), right(x2), bottom(y2) {}

	T width() const { return right - left; }
	T height() const { return bottom - top; }
};

typedef TRect<int>		RectI;
typedef TRect<float>	RectF;

struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
};

inline float getYaw(const glm::vec3 &v) { return std::atan2(v.y, v.x); } // in radians
inline float getPitch(const glm::vec3 &v) { return std::atan2(v.z, std::sqrt(v.x * v.x + v.y * v.y)); } // in radians

inline glm::vec3 vectorToAngles(const glm::vec3 &vDir)
{
	return glm::vec3(0.f, getPitch(vDir), getYaw(vDir));
}

inline glm::vec3 anglesToVector(const glm::vec3 &vAngles)
{
	float cx = std::cos(glm::radians(vAngles.y));
	float sx = std::sin(glm::radians(vAngles.y));
	float cz = std::cos(glm::radians(vAngles.z));
	float sz = std::sin(glm::radians(vAngles.z));
	return glm::vec3(cz*cx, sz*cx, sx);
}
