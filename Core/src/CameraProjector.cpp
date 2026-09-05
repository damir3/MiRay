/*
	Copyright (C) 2013-2020 Damir Sagidullin

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
	documentation files (the "Software"), to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions
	of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
	TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
	THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
	DEALINGS IN THE SOFTWARE.

	(The above is MIT License: http://en.wikipedia.origin/wiki/MIT_License)
*/

#include "CameraProjector.h"

// ------------------------------------------------------------------------ //

class PerspectiveCameraProjector : public ICameraProjector
{
	mat4	m_matViewProj;
	vec3	m_eyePos;
	vec3	m_camDelta[3];
	bool	m_dofEnabled;
	vec2	m_dofLC;
	float	m_dofDP;
	float	m_nearZ;
	float	m_farZ;
	float	m_zNearFarRatio;

public:
	eCameraProjection type() const override { return CP_PERSPECTIVE; }

	void update(Camera & camera) override
	{
		m_matViewProj = camera.viewProjMatrix();
		m_eyePos = glm::translation(camera.matrix());
		auto matViewProjInv = glm::inverse(m_matViewProj);
		auto p = matViewProjInv * vec4(0.f, 0.f, 1.f, 1.f);
		m_camDelta[2] = vec3(p.x, p.y, p.z) / p.w;
		p = matViewProjInv * vec4(1.f, 0.f, 1.f, 1.f);
		m_camDelta[0] = vec3(p.x, p.y, p.z) / p.w - m_camDelta[2];
		p = matViewProjInv * vec4(0.f, -1.f, 1.f, 1.f);
		m_camDelta[1] = vec3(p.x, p.y, p.z) / p.w - m_camDelta[2];

		m_dofEnabled = camera.depthOfField().get();
		auto focusDistance = camera.focusDistance().get();
		auto rayLength = glm::length(m_camDelta[2] - m_eyePos);
		m_dofLC = vec2(focusDistance / rayLength, rayLength / (rayLength - focusDistance));
		m_dofDP = 0.1f / camera.fStop().get();

		m_camDelta[2] -= m_camDelta[0];
		m_camDelta[2] -= m_camDelta[1];
		m_camDelta[0] *= 2.f;
		m_camDelta[1] *= 2.f;

		m_nearZ = camera.nearZ().get();
		m_farZ = camera.farZ().get();
		m_zNearFarRatio = m_nearZ / m_farZ;
	}

	bool getRay(vec3 & origin, vec3 & dir, const vec2 & pos, const vec2 * offset) const override
	{
		origin = m_eyePos;
		auto dest = m_camDelta[2] + m_camDelta[0] * pos.x + m_camDelta[1] * pos.y;
		if (m_dofEnabled && offset) {
			auto p = glm::mix(m_eyePos, dest, m_dofLC.x);
			auto bp = pos + *offset * m_dofDP;
			dest = m_camDelta[2] + m_camDelta[0] * bp.x + m_camDelta[1] * bp.y;
			origin = glm::mix(dest, p, m_dofLC.y);
		}
		dir = dest - origin;

		auto delta = dir * m_zNearFarRatio;
		origin += delta;
		dir -= delta;
		return true;
	}

	bool getViewportCoords(vec2 & out, const vec3 & pos, bool checkBounds) const override
	{
		auto v = m_matViewProj * vec4(pos, 1.f);
		if (v.w <= m_nearZ)
			return false;

		if (checkBounds) {
			if (v.w >= m_farZ)
				return false;

			if (v.x > v.w || v.y > v.w || v.x < -v.w || v.y < -v.w)
				return false;
		}

		out = vec2(v.x, v.y) / v.w;
		return true;
	}
};

// ------------------------------------------------------------------------ //

class OrthographicCameraProjector : public ICameraProjector
{
	mat4	m_matViewProj;
	vec3	m_camDelta[4];

public:
	eCameraProjection type() const override { return CP_ORTHOGRAPHIC; }

	void update(Camera & camera) override
	{
		m_matViewProj = camera.viewProjMatrix();
		auto matViewProjInv = glm::inverse(m_matViewProj);

		auto p = matViewProjInv * vec4(-1.f, 1.f, -1.f, 1.f);
		m_camDelta[3] = vec3(p.x, p.y, p.z) / p.w;

		p = matViewProjInv * vec4(-1.f, 1.f, 1.f, 1.f);
		m_camDelta[2] = vec3(p.x, p.y, p.z) / p.w - m_camDelta[3];

		p = matViewProjInv * vec4(1.f, 1.f, -1.f, 1.f);
		m_camDelta[0] = vec3(p.x, p.y, p.z) / p.w - m_camDelta[3];

		p = matViewProjInv * vec4(-1.f, -1.f, -1.f, 1.f);
		m_camDelta[1] = vec3(p.x, p.y, p.z) / p.w - m_camDelta[3];

		auto delta = m_camDelta[2] * (camera.nearZ().get() / camera.farZ().get());
		m_camDelta[3] += delta;
		m_camDelta[2] -= delta;
	}

	bool getRay(vec3 & origin, vec3 & dir, const vec2 & pos, const vec2 * offset) const override
	{
		origin = m_camDelta[3] + m_camDelta[0] * pos.x + m_camDelta[1] * pos.y;
		dir = m_camDelta[2];
		return true;
	}

	bool getViewportCoords(vec2 & out, const vec3 & pos, bool checkBounds) const override
	{
		auto v = m_matViewProj * vec4(pos, 1.f);

		if (checkBounds) {
			if (v.z < -1.f || v.z > 1.f)
				return false;

			if (v.x > 1.f || v.y > 1.f || v.x < -1.f || v.y < -1.f)
				return false;
		}

		out = vec2(v.x, v.y);
		return true;
	}
};

// ------------------------------------------------------------------------ //

class SphericalCameraProjector : public ICameraProjector
{
	mat4	m_matView;
	mat4	m_mat;
	float	m_zNearFarRatio;

public:
	eCameraProjection type() const override { return CP_SPHERICAL; }

	void update(Camera & camera) override
	{
		auto farZ = camera.farZ().get();
		m_mat = camera.matrix();
		glm::setAxisX(m_mat, glm::axisX(m_mat) * farZ);
		glm::setAxisY(m_mat, glm::axisY(m_mat) * farZ);
		glm::setAxisZ(m_mat, glm::axisZ(m_mat) * farZ);

		m_matView = glm::inverse(m_mat);

		m_zNearFarRatio = camera.nearZ().get() / camera.farZ().get();
	}

	bool getRay(vec3 & origin, vec3 & dir, const vec2 & pos, const vec2 * offset) const override
	{
		auto yaw = pos.x * -M_2PIf;
		auto pitch = pos.y * -M_PIf + M_HALF_PIf;
		auto sx = std::sin(yaw), cx = std::cos(yaw);
		auto sy = std::sin(pitch), cy = std::cos(pitch);
		origin = glm::translation(m_mat);
		dir = glm::axisZ(m_mat) * (cx * cy) + glm::axisX(m_mat) * (sx * cy) + glm::axisY(m_mat) * sy;

		auto delta = dir * m_zNearFarRatio;
		origin += delta;
		dir -= delta;
		return true;
	}

	bool getViewportCoords(vec2 & out, const vec3 & pos, bool checkBounds) const override
	{
		const auto vp = glm::transformCoord(pos, m_matView);
		out.x = std::atan2(vp.x, vp.z) * -M_1_PIf;
		out.x += out.x >= 0.f ? -1.f : 1.f;
		out.y = 1.f - std::atan2(std::sqrt(vp.x * vp.x + vp.z * vp.z), vp.y) * M_2_PIf;
		return true;
	}
};

// ------------------------------------------------------------------------ //

class CylindricalCameraProjector : public ICameraProjector
{
	mat4	m_matView;
	mat4	m_mat;
	float	m_zNearFarRatio;

public:
	eCameraProjection type() const override { return CP_CYLINDRICAL; }

	void update(Camera & camera) override
	{
		auto farZ = camera.farZ().get();
		m_mat = camera.matrix();
		glm::setAxisX(m_mat, glm::axisX(m_mat) * farZ);
		glm::setAxisY(m_mat, glm::axisY(m_mat) * (farZ * M_PIf / camera.aspect().get()));
		glm::setAxisZ(m_mat, glm::axisZ(m_mat) * farZ);

		m_matView = glm::inverse(m_mat);

		m_zNearFarRatio = camera.nearZ().get() / camera.farZ().get();
	}

	bool getRay(vec3 & origin, vec3 & dir, const vec2 & pos, const vec2 * offset) const override
	{
		origin = glm::translation(m_mat);
		auto yaw = pos.x * -M_2PIf;
		dir = glm::axisZ(m_mat) * std::cos(yaw) + glm::axisX(m_mat) * std::sin(yaw) + glm::axisY(m_mat) * (pos.y * -2.f + 1.f);

		auto delta = dir * m_zNearFarRatio;
		origin += delta;
		dir -= delta;

//		auto p1 = glm::transformCoord(origin + dir, m_matView);
		return true;
	}

	bool getViewportCoords(vec2 & out, const vec3 & pos, bool checkBounds) const override
	{
		const auto vp = glm::transformCoord(pos, m_matView);
		out.x = std::atan2(vp.x, vp.z) * -M_1_PIf;
		out.x += out.x >= 0.f ? -1.f : 1.f;
		out.y = vp.y / std::sqrt(vp.x * vp.x + vp.z * vp.z);
		return true;
	}
};

// ------------------------------------------------------------------------ //

class FishEyeCameraProjector : public ICameraProjector
{
	mat4	m_matView;
	mat4	m_mat;
	float	m_fov;
	float	m_zNearFarRatio;

public:
	eCameraProjection type() const override { return CP_FISHEYE; }

	void update(Camera & camera) override
	{
		m_matView = camera.viewMatrix();
		m_mat = camera.matrix();
		glm::setAxisZ(m_mat, glm::axisZ(m_mat) * -camera.farZ().get());

		m_fov = M_PIf;// glm::radians(camera.fov().get());
		m_zNearFarRatio = camera.nearZ().get() / camera.farZ().get();
	}

	bool getRay(vec3 & origin, vec3 & dir, const vec2 & pos, const vec2 * offset) const override
	{
		auto p = pos * 2.f - 1.f;
		auto l = glm::length(p);
		if (l > 1.f)
			return false;

		p *= 1.f / l; // normalize
		origin = glm::translation(m_mat);
		const auto axis = glm::axisX(m_mat) * p.y + glm::axisY(m_mat) * p.x;
		dir = glm::rotate(glm::axisZ(m_mat), -l * m_fov, axis);

		auto delta = dir * m_zNearFarRatio;
		origin += delta;
		dir -= delta;
		return true;
	}

	bool getViewportCoords(vec2 & out, const vec3 & pos, bool checkBounds) const override
	{
		const auto vp = glm::transformCoord(pos, m_matView);
		float l = glm::length(vec2(vp.y, vp.x));
		float d = std::atan2(vp.z, l) * M_1_PIf + 0.5f;
		if (l > 0.f)
			d /= l;

		out.x = vp.x * d;
		out.y = vp.y * d;
		return true;
	}
};

// ------------------------------------------------------------------------ //

ICameraProjector *CreateCameraProjector(eCameraProjection projection)
{
	switch (projection) {
		case CP_PERSPECTIVE:	return new PerspectiveCameraProjector();
		case CP_ORTHOGRAPHIC:	return new OrthographicCameraProjector();
		case CP_SPHERICAL:		return new SphericalCameraProjector();
		case CP_CYLINDRICAL:	return new CylindricalCameraProjector();
		case CP_FISHEYE:		return new FishEyeCameraProjector();
		default:				return nullptr;
	}
}
