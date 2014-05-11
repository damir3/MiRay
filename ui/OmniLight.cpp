//
//  OmniLight.cpp
//  ui
//
//  Created by Damir Sagidullin on 25.08.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#include "OmniLight.h"
#include "SceneUtils.h"

using namespace mr;

// ------------------------------------------------------------------------ //

bool OmniLight::Load(pugi::xml_node node)
{
	ReadVec3(m_origin, "position", node);
	ReadFloat(m_radius, "radius", node);
	ReadVec3(m_intensity, "intensity", node);

	return true;
}

void OmniLight::Save(pugi::xml_node node) const
{
	node = node.append_child("omni-light");

	SaveVec3(m_origin, "position", node);
	SaveFloat(m_radius, "radius", node);
	SaveVec3(m_intensity, "intensity", node);
}

// ------------------------------------------------------------------------ //


Vec3 OmniLight::Intensity(float squared_distance) const
{
	return m_intensity / squared_distance;
}

Vec3 OmniLight::Position(const Vec3 & p) const
{
	Vec3 delta = Vec3::Normalize(Vec3Rand()) * m_radius;
	return Vec3::Dot(delta, p - m_origin) > 0.f ? m_origin + delta : m_origin - delta;
}

Vec3 OmniLight::Intensity(const Vec3 & rayPos, const Vec3 & rayDir) const
{
	Vec3 delta = m_origin - rayPos;
	float C = delta.LengthSquared() - (m_radius * m_radius);
	if (C <= 0.f)
		return Vec3::Null;
	
	float A = rayDir.LengthSquared();
	if (A == 0.f) return Vec3::Null; // degenerate ray
	float B = -2.f * Vec3::Dot(rayDir, delta);
	float D = B * B - 4.f * A * C;
	if (D < 0.f) return Vec3::Null; // not intersect
	float f = (-B - sqrtf(D)) * 0.5f / A;
	return f >= 0.f && f <= 1.f ? m_intensity : Vec3::Null;
}

bool OmniLight::TraceRay(const Vec3 & vFrom, const Vec3 & vTo, TraceResult & tr) const
{
	Vec3 delta = m_origin - vFrom;
	float C = delta.LengthSquared() - (m_radius * m_radius);
	if (C <= 0.f)
		return false;
	
	Vec3 rayDir = vTo - vFrom;
	float A = rayDir.LengthSquared();
	if (A == 0.f)
		return false; // degenerate ray
	
	float B = -2.f * Vec3::Dot(rayDir, delta);
	float D = B * B - 4.f * A * C;
	if (D < 0.f)
		return false; // not intersect
	
	float f = (-B - sqrtf(D)) * 0.5f / A;
	if (f < 0.f || f > 1.f)
		return false;
	
	tr.fraction *= f;
	tr.pos = vFrom + rayDir * f;
	return true;
}

BBoxF OmniLight::OOBB() const
{
	return BBox(Vec3(-m_radius), Vec3(m_radius));
}

Matrix OmniLight::Transformation() const
{
	Matrix mat;
	mat.Translation(m_origin);
	return mat;
}

void OmniLight::SetTransformation(const Matrix & mat)
{
	m_origin = mat.Pos();
	m_radius *= fmaxf(fmaxf(mat.AxisX().Length(), mat.AxisY().Length()), mat.AxisZ().Length());
}

void OmniLight::Draw()
{
	float mi = fmaxf(fmaxf(m_intensity.x, m_intensity.y), m_intensity.z);
	ColorF c = m_intensity / mi;
	c.a = 0.5f;
	DrawGeosphere(m_origin, m_radius, c, 2);
}
