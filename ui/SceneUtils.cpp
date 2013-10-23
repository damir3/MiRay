//
//  RenderUtils.cpp
//  MiRay/ui
//
//  Created by Damir Sagidullin on 16.08.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#include "SceneUtils.h"
#include "../rt/CollisionNode.h"

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
	Vec3 vDirZ = Vec3::Normalize(p2 - p1);
	Vec3 vDirX = vDirZ.Perpendicular();
	Vec3 vDirY = Vec3::Cross(vDirZ, vDirX);
	float da = M_2PIf / num;
	glBegin(GL_TRIANGLES);
	for (int i = 0; i < num; i++)
	{
		float a1 = i * da;
		Vec3 d1 = vDirX * cosf(a1) + vDirY * sinf(a1);
		Vec3 p11 = p1 + d1 * radius;
		Vec3 p21 = p2 + d1 * radius;
		float a2 = (i+1) * da;
		Vec3 d2 = vDirX * cosf(a2) + vDirY * sinf(a2);
		Vec3 p12 = p1 + d2 * radius;
		Vec3 p22 = p2 + d2 * radius;
		
		glColor4ubv(VertexColor(c, -vDirZ));
		glVertex3fv(p1);
		glVertex3fv(p11);
		glVertex3fv(p12);
		
		glColor4ubv(VertexColor(c, vDirZ));
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
	Vec3 vDirZ = p2 - p1;
	float length = vDirZ.Normalize();
	Vec3 vDirX = vDirZ.Perpendicular();
	Vec3 vDirY = Vec3::Cross(vDirZ, vDirX);
	float g = sqrtf(radius * radius + length * length);
	float f1 = radius / g;
	float f2 = length / g;
	float da = M_2PIf / num;
	glBegin(GL_TRIANGLES);
	for (int i = 0; i < num; i++)
	{
		float a1 = i * da;
		Vec3 d1 = vDirX * cosf(a1) + vDirY * sinf(a1);
		Vec3 pa1 = p1 + d1 * radius;
		float a2 = (i+1) * da;
		Vec3 d2 = vDirX * cosf(a2) + vDirY * sinf(a2);
		Vec3 pa2 = p1 + d2 * radius;
		
		glColor4ubv(VertexColor(c, -vDirZ));
		glVertex3fv(p1);
		glVertex3fv(pa1);
		glVertex3fv(pa2);
		
		Vec3 na1 = vDirZ * f1 + d1 * f2;
		Vec3 na2 = vDirZ * f1 + d2 * f2;
		glColor4ubv(VertexColor(c, Vec3::Normalize(na1 + na2)));
		glVertex3fv(p2);
		glColor4ubv(VertexColor(c, na2));
		glVertex3fv(pa2);
		glColor4ubv(VertexColor(c, na1));
		glVertex3fv(pa1);
	}
	glEnd();
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
