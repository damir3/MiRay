//
//  RenderUtils.cpp
//  MiRay/ui
//
//  Created by Damir Sagidullin on 16.08.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#include "SceneUtils.h"
#include "../rt/CollisionNode.h"
#ifdef _WIN32
#define	sscanf		sscanf_s
#endif // _WIN32


namespace mr
{

void DrawGrid()
{
	glDisable(GL_BLEND);
	glDisable(GL_LINE_SMOOTH);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glEnable(GL_LINE_SMOOTH);
	//glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glLineWidth(1.f);
	
	//	glDepthMask(GL_FALSE);
	
	glBegin(GL_LINES);
	glColor4f(0.75f, 0.75f, 0.75f, 1.f);
	for (int x = -9; x < 10; x++)
	{
		if (x == 0) continue;
		glVertex3fv(Vec3((float)x, -10.f, 0.f));
		glVertex3fv(Vec3((float)x, 10.f, 0.f));
		glVertex3fv(Vec3(-10.f, (float)x, 0.f));
		glVertex3fv(Vec3(10.f, (float)x, 0.f));
	}
	glColor4f(0.5f, 0.5f, 0.5f, 1.f);
	for (int x = -9; x < 10; x++)
	{
		glVertex3fv(Vec3(x * 10.f, -100.f, 0.f));
		glVertex3fv(Vec3(x * 10.f, 100.f, 0.f));
		glVertex3fv(Vec3(-100.f, x * 10.f, 0.f));
		glVertex3fv(Vec3(100.f, x * 10.f, 0.f));
	}
	
	glVertex3fv(Vec3(100.f, 100.f, 0.f));
	glVertex3fv(Vec3(100.f, -100.f, 0.f));
	
	glVertex3fv(Vec3(100.f, 100.f, 0.f));
	glVertex3fv(Vec3(-100.f, 100.f, 0.f));
	
	glVertex3fv(Vec3(-100.f, -100.f, 0.f));
	glVertex3fv(Vec3(100.f, -100.f, 0.f));
	
	glVertex3fv(Vec3(-100.f, -100.f, 0.f));
	glVertex3fv(Vec3(-100.f, 100.f, 0.f));
	
	glColor3f(1.f, 0.f, 0.f);
	glVertex3fv(Vec3::Null);
	glVertex3fv(Vec3(100.f, 0.f, 0.f));
	
	glColor3f(0.f, 1.f, 0.f);
	glVertex3fv(Vec3::Null);
	glVertex3fv(Vec3(0.f, 100.f, 0.f));
	
	glColor3f(0.f, 0.f, 1.f);
	glVertex3fv(Vec3::Null);
	glVertex3fv(Vec3(0.f, 0.f, 100.f));
	
	glEnd();
	
	//	glDepthMask(GL_TRUE);
	//glDisable(GL_LINE_SMOOTH);
}

void DrawWireframeBox(const BBox & box, const Color & c)
{
	glDisable(GL_BLEND);
	glDisable(GL_LINE_SMOOTH);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glEnable(GL_LINE_SMOOTH);
	//glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glLineWidth(1.f);
	
	glBegin(GL_LINES);
	
	glColor4ubv(c);
	
	glVertex3fv(Vec3(box.vMins.x, box.vMins.y, box.vMins.z));
	glVertex3fv(Vec3(box.vMaxs.x, box.vMins.y, box.vMins.z));
	
	glVertex3fv(Vec3(box.vMins.x, box.vMaxs.y, box.vMins.z));
	glVertex3fv(Vec3(box.vMaxs.x, box.vMaxs.y, box.vMins.z));
	
	glVertex3fv(Vec3(box.vMins.x, box.vMins.y, box.vMaxs.z));
	glVertex3fv(Vec3(box.vMaxs.x, box.vMins.y, box.vMaxs.z));
	
	glVertex3fv(Vec3(box.vMins.x, box.vMaxs.y, box.vMaxs.z));
	glVertex3fv(Vec3(box.vMaxs.x, box.vMaxs.y, box.vMaxs.z));
	
	glVertex3fv(Vec3(box.vMins.x, box.vMins.y, box.vMins.z));
	glVertex3fv(Vec3(box.vMins.x, box.vMaxs.y, box.vMins.z));
	
	glVertex3fv(Vec3(box.vMaxs.x, box.vMins.y, box.vMins.z));
	glVertex3fv(Vec3(box.vMaxs.x, box.vMaxs.y, box.vMins.z));
	
	glVertex3fv(Vec3(box.vMins.x, box.vMins.y, box.vMaxs.z));
	glVertex3fv(Vec3(box.vMins.x, box.vMaxs.y, box.vMaxs.z));
	
	glVertex3fv(Vec3(box.vMaxs.x, box.vMins.y, box.vMaxs.z));
	glVertex3fv(Vec3(box.vMaxs.x, box.vMaxs.y, box.vMaxs.z));
	
	glVertex3fv(Vec3(box.vMins.x, box.vMins.y, box.vMins.z));
	glVertex3fv(Vec3(box.vMins.x, box.vMins.y, box.vMaxs.z));
	
	glVertex3fv(Vec3(box.vMaxs.x, box.vMins.y, box.vMins.z));
	glVertex3fv(Vec3(box.vMaxs.x, box.vMins.y, box.vMaxs.z));
	
	glVertex3fv(Vec3(box.vMins.x, box.vMaxs.y, box.vMins.z));
	glVertex3fv(Vec3(box.vMins.x, box.vMaxs.y, box.vMaxs.z));
	
	glVertex3fv(Vec3(box.vMaxs.x, box.vMaxs.y, box.vMins.z));
	glVertex3fv(Vec3(box.vMaxs.x, box.vMaxs.y, box.vMaxs.z));
	
	glEnd();
}

void DrawBox(const BBox & bbox, const Color & c)
{
	glBegin(GL_QUADS);
	glColor4ubv(VertexColor(c, -Vec3::X));
	glVertex3fv(Vec3(bbox.vMins.x, bbox.vMaxs.y, bbox.vMaxs.z));
	glVertex3fv(Vec3(bbox.vMins.x, bbox.vMins.y, bbox.vMaxs.z));
	glVertex3fv(Vec3(bbox.vMins.x, bbox.vMins.y, bbox.vMins.z));
	glVertex3fv(Vec3(bbox.vMins.x, bbox.vMaxs.y, bbox.vMins.z));
	
	glColor4ubv(VertexColor(c, Vec3::X));
	glVertex3fv(Vec3(bbox.vMaxs.x, bbox.vMins.y, bbox.vMaxs.z));
	glVertex3fv(Vec3(bbox.vMaxs.x, bbox.vMaxs.y, bbox.vMaxs.z));
	glVertex3fv(Vec3(bbox.vMaxs.x, bbox.vMaxs.y, bbox.vMins.z));
	glVertex3fv(Vec3(bbox.vMaxs.x, bbox.vMins.y, bbox.vMins.z));
	
	glColor4ubv(VertexColor(c, -Vec3::Y));
	glVertex3fv(Vec3(bbox.vMins.x, bbox.vMins.y, bbox.vMaxs.z));
	glVertex3fv(Vec3(bbox.vMaxs.x, bbox.vMins.y, bbox.vMaxs.z));
	glVertex3fv(Vec3(bbox.vMaxs.x, bbox.vMins.y, bbox.vMins.z));
	glVertex3fv(Vec3(bbox.vMins.x, bbox.vMins.y, bbox.vMins.z));
	
	glColor4ubv(VertexColor(c, Vec3::Y));
	glVertex3fv(Vec3(bbox.vMaxs.x, bbox.vMaxs.y, bbox.vMaxs.z));
	glVertex3fv(Vec3(bbox.vMins.x, bbox.vMaxs.y, bbox.vMaxs.z));
	glVertex3fv(Vec3(bbox.vMins.x, bbox.vMaxs.y, bbox.vMins.z));
	glVertex3fv(Vec3(bbox.vMaxs.x, bbox.vMaxs.y, bbox.vMins.z));
	
	glColor4ubv(VertexColor(c, -Vec3::Z));
	glVertex3fv(Vec3(bbox.vMins.x, bbox.vMins.y, bbox.vMins.z));
	glVertex3fv(Vec3(bbox.vMaxs.x, bbox.vMins.y, bbox.vMins.z));
	glVertex3fv(Vec3(bbox.vMaxs.x, bbox.vMaxs.y, bbox.vMins.z));
	glVertex3fv(Vec3(bbox.vMins.x, bbox.vMaxs.y, bbox.vMins.z));
	
	glColor4ubv(VertexColor(c, Vec3::Z));
	glVertex3fv(Vec3(bbox.vMaxs.x, bbox.vMins.y, bbox.vMaxs.z));
	glVertex3fv(Vec3(bbox.vMins.x, bbox.vMins.y, bbox.vMaxs.z));
	glVertex3fv(Vec3(bbox.vMins.x, bbox.vMaxs.y, bbox.vMaxs.z));
	glVertex3fv(Vec3(bbox.vMaxs.x, bbox.vMaxs.y, bbox.vMaxs.z));
	glEnd();
}

void DrawLine(const Vec3 & v1, const Vec3 & v2, const Color & c, float w)
{
	glBegin(GL_LINES);
	glColor4ubv(c);
	glVertex3fv(v1);
	glVertex3fv(v2);
	glEnd();
}

const float a = 2.f / (1.f + sqrtf(5.f));
const float b = 1.f / sqrtf((3.f + sqrtf(5.f)) / (1.f + sqrtf(5.f)));
static const Vec3 geospherePoints[] = {
	Vec3( 0,  a,  b),
	Vec3( 0,  a, -b),
	Vec3( 0, -a,  b),
	Vec3( 0, -a, -b),
	Vec3( a,  b,  0),
	Vec3( a, -b,  0),
	Vec3(-a,  b,  0),
	Vec3(-a, -b,  0),
	Vec3( b,  0,  a),
	Vec3( b,  0, -a),
	Vec3(-b,  0,  a),
	Vec3(-b,  0, -a)
};
static const uint32 geosphereIndices[] =
{
	1, 4, 6,	0, 6, 4,
	0, 2, 10,	0, 8, 2,
	1, 3, 9,	1, 11, 3,
	2, 5, 7,	3, 7, 5,
	6, 10, 11,	7, 11, 10,
	4, 9, 8,	5, 8, 9,
	0, 10, 6,	0, 4, 8,
	1, 6, 11,	1, 9, 4,
	3, 11, 7,	3, 5, 9,
	2, 7, 10,	2, 8, 5,
};

void DrawSubDivSphereTriangle(const Vec3 & pos, const Vec3 & p1, const Vec3 & p2, const Vec3 & p3, float r, const Color & c, int lod)
{
	if (lod == 0)
	{
		glColor4ubv(VertexColor(c, p1));
		glVertex3fv(pos + p1 * r);
		glColor4ubv(VertexColor(c, p2));
		glVertex3fv(pos + p2 * r);
		glColor4ubv(VertexColor(c, p3));
		glVertex3fv(pos + p3 * r);
	}
	else
	{
		Vec3 p12 = Vec3::Normalize(p1 + p2);
		Vec3 p23 = Vec3::Normalize(p2 + p3);
		Vec3 p31 = Vec3::Normalize(p3 + p1);
		lod--;
		DrawSubDivSphereTriangle(pos, p1, p12, p31, r, c, lod);
		DrawSubDivSphereTriangle(pos, p2, p23, p12, r, c, lod);
		DrawSubDivSphereTriangle(pos, p3, p31, p23, r, c, lod);
		DrawSubDivSphereTriangle(pos, p12, p23, p31, r, c, lod);
	}
}

void DrawGeosphere(const Vec3 & pos, float r, const Color & c, int lod)
{
	glBegin(GL_TRIANGLES);
	for (int i = 0; i < 60; i += 3)
	{
		DrawSubDivSphereTriangle(pos,
								 geospherePoints[geosphereIndices[i]],
								 geospherePoints[geosphereIndices[i+1]],
								 geospherePoints[geosphereIndices[i+2]],
								 r, c, lod);
	}
	glEnd();
}

void DrawCylinder(const Vec3 & p1, const Vec3 & p2, float radius, const Color & c, int num)
{
	Vec3 dirZ = Vec3::Normalize(p2 - p1);
	Vec3 dirX = dirZ.Perpendicular();
	Vec3 dirY = Vec3::Cross(dirZ, dirX);
	float da = M_2PIf / num;
	glBegin(GL_TRIANGLES);
	for (int i = 0; i < num; i++)
	{
		float a1 = i * da;
		Vec3 d1 = dirX * cosf(a1) + dirY * sinf(a1);
		Vec3 p11 = p1 + d1 * radius;
		Vec3 p21 = p2 + d1 * radius;
		float a2 = (i+1) * da;
		Vec3 d2 = dirX * cosf(a2) + dirY * sinf(a2);
		Vec3 p12 = p1 + d2 * radius;
		Vec3 p22 = p2 + d2 * radius;
		
		glColor4ubv(VertexColor(c, -dirZ));
		glVertex3fv(p1);
		glVertex3fv(p11);
		glVertex3fv(p12);
		
		glColor4ubv(VertexColor(c, dirZ));
		glVertex3fv(p2);
		glVertex3fv(p22);
		glVertex3fv(p21);
		
		glColor4ubv(VertexColor(c, d1));
		glVertex3fv(p11);
		glVertex3fv(p21);
		glColor4ubv(VertexColor(c, d2));
		glVertex3fv(p22);
		
		glColor4ubv(VertexColor(c, d2));
		glVertex3fv(p22);
		glVertex3fv(p12);
		glColor4ubv(VertexColor(c, d1));
		glVertex3fv(p11);
	}
	glEnd();
}

void DrawCone(const Vec3 & p1, const Vec3 & p2, float radius, const Color & c, int num)
{
	Vec3 dirZ = p2 - p1;
	float length = dirZ.Normalize();
	Vec3 dirX = dirZ.Perpendicular();
	Vec3 dirY = Vec3::Cross(dirZ, dirX);
	float g = sqrtf(radius * radius + length * length);
	float f1 = radius / g;
	float f2 = length / g;
	float da = M_2PIf / num;

	glBegin(GL_TRIANGLES);
	for (int i = 0; i < num; i++)
	{
		float a1 = i * da;
		Vec3 d1 = dirX * cosf(a1) + dirY * sinf(a1);
		Vec3 pa1 = p1 + d1 * radius;
		float a2 = (i+1) * da;
		Vec3 d2 = dirX * cosf(a2) + dirY * sinf(a2);
		Vec3 pa2 = p1 + d2 * radius;
		
		glColor4ubv(VertexColor(c, -dirZ));
		glVertex3fv(p1);
		glVertex3fv(pa1);
		glVertex3fv(pa2);
		
		Vec3 na1 = dirZ * f1 + d1 * f2;
		Vec3 na2 = dirZ * f1 + d2 * f2;
		glColor4ubv(VertexColor(c, Vec3::Normalize(na1 + na2)));
		glVertex3fv(p2);
		glColor4ubv(VertexColor(c, na2));
		glVertex3fv(pa2);
		glColor4ubv(VertexColor(c, na1));
		glVertex3fv(pa1);
	}
	glEnd();
}

void DrawArrow(const Vec3 & pos, const Vec3 & dir, const Color & c, float l)
{
	float r1 = l * 0.02f;
	float r2 = l * 0.04f;
	
	Vec3 p1 = pos + dir * (l * 0.7f);
	Vec3 p2 = pos + dir * l;
	//DrawLine(pos, p1, c, 1.f);
	DrawCylinder(pos, p1, r1, c, 8);
	DrawCone(p1, p2, r2, c, 16);
}

void DrawCircle(const Vec3 & p, const Vec3 & normal, float r1, float r2, const Color & c1, const Color & c2, float borderLineWidth)
{
	enum {NUM_SEGMENTS = 64};
	Vec3 points[2][NUM_SEGMENTS];
	Vec3 dirX = normal.Perpendicular();
	Vec3 dirY = Vec3::Cross(normal, dirX);
	for (int i = 0; i < NUM_SEGMENTS; i++)
	{
		float a = i * (M_2PIf / NUM_SEGMENTS);
		float ca = cosf(a), sa = sinf(a);
		points[0][i] = p + dirX * (r1 * ca) + dirY * (r1 * sa);
		points[1][i] = p + dirX * (r2 * ca) + dirY * (r2 * sa);
	}
	
	if (c2.a > 0 && r1 != r2)
	{
		glColor4ubv(c2);
		glDisable(GL_CULL_FACE);
		glBegin(GL_TRIANGLE_STRIP);
		for (int i = 0; i < NUM_SEGMENTS; i++)
		{
			glVertex3fv(points[0][i]);
			glVertex3fv(points[1][i]);
		}
		glVertex3fv(points[0][0]);
		glVertex3fv(points[1][0]);
		glEnd();
		glEnable(GL_CULL_FACE);
		
	}
	
	if (c1.a > 0 && borderLineWidth > 0.f)
	{
		glEnable(GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		glLineWidth(borderLineWidth);
		
		glColor4ubv(c1);
		glBegin(GL_LINE_LOOP);
		for (int i = 0; i < NUM_SEGMENTS; i++)
			glVertex3fv(points[0][i]);

		glVertex3fv(points[0][0]);
		glEnd();
		if (r1 != r2)
		{
			glBegin(GL_LINE_LOOP);
			for (int i = 0; i < NUM_SEGMENTS; i++)
				glVertex3fv(points[1][i]);

			glVertex3fv(points[1][0]);
			glEnd();
		}
		
		glDisable(GL_LINE_SMOOTH);
	}
}

void DrawCollisionNode(const CollisionNode * pCN, byte level)
{
	if (!pCN)
		return;

	static Color colors[] = {Color(255, 0, 0), Color(0, 255, 0), Color(0, 0, 255), Color(255, 255, 0), Color(255, 0, 255), Color(0, 255, 255), Color(255, 128, 0)};
	DrawWireframeBox(pCN->BoundingBox(), colors[level % 7]);

//	const sTriangleCutArray & triangles = pCN->Triangles();
//	if (!triangles.empty())
//	{
//		Color c(255, 255, 255);
//		glBegin(GL_TRIANGLES);
//		for (sTriangleCutArray::const_iterator it = triangles.begin(), itEnd = triangles.end(); it != itEnd; ++it)
//		{
//			glColor3ubv(VertexColor(c, it->v[1]));
//			glVertex3fv(it->v[0]);
//			glColor3ubv(VertexColor(c, it->v[1]));
//			glVertex3fv(it->v[1]);
//			glColor3ubv(VertexColor(c, it->v[2]));
//			glVertex3fv(it->v[2]);
//		}
//		glEnd();
//
//		glPolygonOffset(-4.f, -4.f);
//		glPolygonMode(GL_FRONT, GL_LINE);
//		glEnable(GL_POLYGON_OFFSET_LINE);
//		glDepthMask(GL_FALSE);
//
//		glColor4ub(0, 128, 255, 255);
//		glBegin(GL_TRIANGLES);
//		for (sTriangleCutArray::const_iterator it = triangles.begin(), itEnd = triangles.end(); it != itEnd; ++it)
//		{
//			glVertex3fv(it->v[0]);
//			glVertex3fv(it->v[1]);
//			glVertex3fv(it->v[2]);
//		}
//		glEnd();
//
//		glDepthMask(GL_TRUE);
//		glDisable(GL_POLYGON_OFFSET_LINE);
//		glPolygonMode(GL_FRONT, GL_FILL);
//	}

	DrawCollisionNode(pCN->Child(0), level + 1);
	DrawCollisionNode(pCN->Child(1), level + 1);
}

// ------------------------------------------------------------------------ //

bool GetRayAxisIntersectionDelta(const Vec3 & rayStart, const Vec3 & rayEnd, const Vec3 & axisPos, const Vec3 & axisDir, float l, float t, Vec3 & delta)
{
	Vec3 rayDir = rayEnd - rayStart;
	Vec3 normal = Vec3::Cross(rayDir, axisDir);
	if (normal.Normalize() == 0.f)
		return false;
	
	if (t > 0.f)
	{
		float d = Vec3::Dot(normal, axisPos - rayStart);
		if (fabsf(d) > t)
			return false;
	}
	
	Vec3 dir = Vec3::Cross(normal, rayDir);
	float dist = Vec3::Dot(dir, rayStart - axisPos) / Vec3::Dot(dir, axisDir);
	if (l > 0.f && (dist < 0.f || dist > l))
		return false;
	
	delta = axisDir * dist;
	return true;
}
	
bool GetRayPlaneIntersectionDelta(const Vec3 & rayStart, const Vec3 & rayEnd, const Vec3 & axisPos, const Vec3 & axisDir, Vec3 & delta)
{
	float d1 = Vec3::Dot(axisPos - rayStart, axisDir);
	float d2 = Vec3::Dot(axisPos - rayEnd, axisDir);
	if ((d1 > 0.f) ^ (d2 <= 0.f))
		return false;

	Vec3 rayDir = rayEnd - rayStart;
	delta = rayStart + rayDir * (d1 / Vec3::Dot(rayDir, axisDir)) - axisPos;
	return true;
}

float GetRayBoxIntersection(const Vec3 & rayStart, const Vec3 & rayEnd, const BBox & box)
{
	float tmin = 0.f;
	float tmax = 1.f;
	for (int i = 0; i < 3; i++)
	{
		float delta = rayEnd[i] - rayStart[i];
		if (delta == 0.f)
			continue;

		float invDelta = 1.f / delta;
		float t1, t2;
		if (delta > 0.f)
		{
			t1 = (box.vMins[i] - rayStart[i]) * invDelta;
			t2 = (box.vMaxs[i] - rayStart[i]) * invDelta;
		}
		else
		{
			t1 = (box.vMaxs[i] - rayStart[i]) * invDelta;
			t2 = (box.vMins[i] - rayStart[i]) * invDelta;
		}
		if ((tmin > t2) || (t1 > tmax))
			return 1.f;

		tmin = std::max(tmin, t1);
		tmax = std::min(tmax, t2);
	}

	return tmin;
}

// ------------------------------------------------------------------------ //

void SaveFloat(float f, const char * name, pugi::xml_node node)
{
	char buf[256];
	snprintf(buf, sizeof(buf), "%g", f);
	node.append_child(name).text().set(buf);
}

void SaveVec3(const Vec3 & v, const char * name, pugi::xml_node node)
{
	char buf[256];
	snprintf(buf, sizeof(buf), "%g %g %g", v.x, v.y, v.z);
	node.append_child(name).text().set(buf);
}

void ReadFloat(float & f, const char * name, pugi::xml_node node)
{
	pugi::xml_node value = node.child(name);
	if (!value.empty())
	{
		const char * str = value.text().get();
		if (str && *str)
			sscanf(str, "%f", &f);
	}
}

void ReadVec3(Vec3 & v, const char * name, pugi::xml_node node)
{
	pugi::xml_node value = node.child(name);
	if (!value.empty())
	{
		const char * str = value.text().get();
		if (str && *str)
		{
			int n = sscanf(str, "%f %f %f", &v.x, &v.y, &v.z);
			if (n == 1)
				v.y = v.z = v.x;
			else if (n == 2)
				v.z = 0.f;
		}
	}
}

}
