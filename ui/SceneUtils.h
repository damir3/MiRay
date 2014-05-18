//
//  RenderUtils.h
//  MiRay/ui
//
//  Created by Damir Sagidullin on 16.08.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

namespace mr
{
	
void DrawGrid();
void DrawWireframeBox(const BBox & box, const Color & c);
void DrawBox(const BBox & box, const Color & c);
void DrawLine(const Vec3 & p1, const Vec3 & p2, const Color & c, float w);
void DrawGeosphere(const Vec3 & pos, float r, const Color & c, int lod);
void DrawCylinder(const Vec3 & p1, const Vec3 & p2, float radius, const Color & c, int num);
void DrawCone(const Vec3 & p1, const Vec3 & p2, float radius, const Color & c, int num);
void DrawArrow(const Vec3 & pos, const Vec3 & dir, const Color & c, float l);
void DrawCircle(const Vec3 & p, const Vec3 & normal, float r1, float r2, const Color & c1, const Color & c2, float borderLineWidth);
void DrawGizmo(const Matrix & mat, const BBox & bbox);
void DrawCollisionNode(const class CollisionNode * pCN, byte level);

void SaveFloat(float v, const char * name, pugi::xml_node node);
void SaveVec3(const Vec3 & v, const char * name, pugi::xml_node node);
void ReadFloat(float & v, const char * name, pugi::xml_node node);
void ReadVec3(Vec3 & v, const char * name, pugi::xml_node node);

}
