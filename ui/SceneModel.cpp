//
//  SceneModel.cpp
//  MiRay/ui
//
//  Created by Damir Sagidullin on 25.08.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#include "SceneModel.h"
#include "../resources/Model.h"
#include "SceneUtils.h"

using namespace mr;

// ------------------------------------------------------------------------ //

SceneModel::SceneModel(BVH &bvh)
	: m_bvh(bvh)
	, m_pModel(NULL)
	, m_pVolume(NULL)
{
}

SceneModel::~SceneModel()
{
	m_bvh.DestroyVolume(m_pVolume);

	while (!m_meshes.empty())
	{
		delete m_meshes.back();
		m_meshes.pop_back();
	}

	SAFE_RELEASE(m_pModel);
}

// ------------------------------------------------------------------------ //

bool SceneModel::Init(const char *pFilename, const Matrix &mat, ModelManager *pModelManager, pugi::xml_node node)
{
	printf("Loading model '%s'...\n", pFilename);
	
	double tm1 = Timer::GetSeconds();

	m_pModel = pModelManager->LoadModel(pFilename, node);
	if (!m_pModel)
		return false;

	double tm2 = Timer::GetSeconds();
	printf("Model loading time: %f ms\n", (tm2 - tm1) * 1000.0);

	Vec3 vCenter = m_pModel->BoundingBox().Center();
	Vec3 vSize = m_pModel->BoundingBox().Size();
	printf("center(%g %g %g) size(%g %g %g)\n", vCenter.x, vCenter.y, vCenter.z, vSize.x, vSize.y, vSize.z);

	m_meshes.reserve(m_pModel->Meshes().size());
	for (size_t i = 0; i < m_pModel->Meshes().size(); i++)
		m_meshes.push_back(new RenderMesh(*m_pModel->Meshes()[i]));

	double tm3 = Timer::GetSeconds();
	
	size_t numTriangles = 0;
	for (auto itMesh = m_pModel->Meshes().begin(); itMesh != m_pModel->Meshes().end(); ++itMesh)
	{
		for (auto itGeom = (*itMesh)->m_geometries.begin(); itGeom != (*itMesh)->m_geometries.end(); ++itGeom)
			numTriangles += (*itGeom)->m_indices.size() / 3;
	}
	
	m_pVolume = m_bvh.CreateVolume(numTriangles);
	m_pVolume->SetTransformation(mat);
	
	for (auto itMesh = m_pModel->Meshes().begin(); itMesh != m_pModel->Meshes().end(); ++itMesh)
	{
		for (auto itGeom = (*itMesh)->m_geometries.begin(); itGeom != (*itMesh)->m_geometries.end(); ++itGeom)
		{
			Model::Geometry & geom = *(*itGeom);
			for (const uint32 *pInd = &geom.m_indices[0], *pIndEnd = pInd + geom.m_indices.size(); pInd < pIndEnd; pInd += 3)
			{
				CollisionTriangle t(geom.m_vertices[pInd[0]], geom.m_vertices[pInd[1]], geom.m_vertices[pInd[2]],
									geom.m_pMaterial);
				m_pVolume->AddTriangle(t);
			}
		}
	}
	
	m_pVolume->Build(30);

	double tm4 = Timer::GetSeconds();
	printf("Collision scene creating time: %ld triangles, %d nodes, %d, %f ms\n", m_pVolume->Triangles().size(),
		   static_cast<int>(m_pVolume->Root()->GetNodeCount()),
		   static_cast<int>(m_pVolume->Root()->GetChildrenDepth()),
		   (tm4 - tm3) * 1000.0);
	return true;
}

// ------------------------------------------------------------------------ //

bool SceneModel::Load(pugi::xml_node node, ModelManager *pModelManager)
{
	pugi::xml_node filename = node.child("filename");
	if (filename.empty())
		return false;

	Matrix mat;
	Vec3 position(0.f);
	Vec3 rotation(0.f);
	Vec3 scale(1.f);

	pugi::xml_node transformation = node.child("transformation");
	if (!transformation.empty())
	{
		ReadVec3(position, "position", transformation);
		ReadVec3(rotation, "rotation", transformation);
		ReadVec3(scale, "scale", transformation);
	}

	mat = MatrixBuilder().AddScale(scale).AddRotation(rotation).AddPosition(position);

	std::string strFilename = filename.text().get();
	return Init(strFilename.c_str(), mat, pModelManager, node);
}

void SceneModel::Save(pugi::xml_node node) const
{
	node = node.append_child("model");
	node.append_child("filename").text().set(m_pModel->Name());
	pugi::xml_node transformation = node.append_child("transformation");
	const Matrix & mat = m_pVolume->Transformation();
	SaveVec3(mat.Pos(), "position", transformation);
	SaveVec3(mat.Rotation(), "rotation", transformation);
	SaveVec3(mat.Scale(), "scale", transformation);
	m_pModel->Save(node);
}

// ------------------------------------------------------------------------ //

BBoxF SceneModel::OOBB() const { return m_pVolume->OOBB(); }
Matrix SceneModel::Transformation() const { return m_pVolume->Transformation(); }
void SceneModel::SetTransformation(const Matrix & mat) { m_pVolume->SetTransformation(mat); }

// ------------------------------------------------------------------------ //

void SceneModel::Draw()
{
	for (auto it = m_meshes.begin(), itEnd = m_meshes.end(); it != itEnd; ++it)
		(*it)->Draw();
}

void SceneModel::DrawWireframe()
{
	for (auto it = m_meshes.begin(), itEnd = m_meshes.end(); it != itEnd; ++it)
		(*it)->DrawWireframe();
}

void SceneModel::DrawNormals(float l)
{
	for (auto it = m_meshes.begin(), itEnd = m_meshes.end(); it != itEnd; ++it)
		(*it)->DrawNormals(l);
}

// ------------------------------------------------------------------------ //

SceneModel::RenderMesh::RenderMesh(Model::Mesh & mesh)
	: m_mesh(mesh)
{
	m_geometries.reserve(mesh.m_geometries.size());
	for (size_t i = 0; i < mesh.m_geometries.size(); i++)
		m_geometries.push_back(new RenderGeometry(*mesh.m_geometries[i]));
}

SceneModel::RenderMesh::~RenderMesh()
{
	while (!m_geometries.empty())
	{
		delete m_geometries.back();
		m_geometries.pop_back();
	}
}

void SceneModel::RenderMesh::Draw()
{
	for (auto it = m_geometries.begin(), itEnd = m_geometries.end(); it != itEnd; ++it)
		(*it)->Draw();
}

void SceneModel::RenderMesh::DrawWireframe()
{
	for (auto it = m_geometries.begin(), itEnd = m_geometries.end(); it != itEnd; ++it)
		(*it)->DrawWireframe();
}

void SceneModel::RenderMesh::DrawNormals(float l)
{
	for (auto it = m_geometries.begin(), itEnd = m_geometries.end(); it != itEnd; ++it)
		(*it)->DrawNormals(l);
}

// ------------------------------------------------------------------------ //

struct sArrayBufferVertex
{
	Vec3 pos;
	Color color;
};

SceneModel::RenderGeometry::RenderGeometry(Model::Geometry & geom)
	: m_geometry(geom)
	, m_vertexCount(static_cast<GLsizei>(geom.m_vertices.size()))
	, m_indexCount(static_cast<GLsizei>(geom.m_indices.size()))
	, m_vertexBuffer(0)
	, m_indexBuffer(0)
{
	glGenBuffers(1, &m_vertexBuffer);
	if (m_vertexBuffer > 0)
	{
		std::vector<sArrayBufferVertex> vertices(geom.m_vertices.size());
		for (size_t i = 0; i < vertices.size(); i++)
		{
			vertices[i].pos = geom.m_vertices[i].pos;
			vertices[i].color = VertexColor(Color::White, geom.m_vertices[i].normal);
		}
		glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, m_vertexCount * sizeof(sArrayBufferVertex), vertices.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	
	glGenBuffers(1, &m_indexBuffer);
	if (m_indexBuffer > 0)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indexCount * sizeof(uint32), geom.m_indices.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
}

SceneModel::RenderGeometry::~RenderGeometry()
{
	if (m_vertexBuffer)
	{
		glDeleteBuffers(1, &m_vertexBuffer);
		m_vertexBuffer = 0;
	}

	if (m_indexBuffer)
	{
		glDeleteBuffers(1, &m_indexBuffer);
		m_indexBuffer = 0;
	}
}

void SceneModel::RenderGeometry::Draw()
{
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(sArrayBufferVertex), (void*)0);
	glEnableClientState(GL_COLOR_ARRAY);
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(sArrayBufferVertex), (void*)12);
//	glEnableClientState(GL_NORMAL_ARRAY);
//	glNormalPointer(GL_FLOAT, sizeof(sArrayBufferVertex), (void*)12);
//	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
//	glTexCoordPointer(2, GL_FLAT, sizeof(sArrayBufferVertex), (void*)48);
	
	glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, 0);
	
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void SceneModel::RenderGeometry::DrawWireframe()
{
	glLineWidth(1.f);
	
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(sArrayBufferVertex), (void*)0);
	
//	glEnable(GL_BLEND);
//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//	glEnable(GL_LINE_SMOOTH);
//	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
//	glLineWidth(0.5f);
	glPolygonOffset(-4.f, -4.f);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glEnable(GL_POLYGON_OFFSET_LINE);
	glDepthMask(GL_FALSE);
	
	glColor4ub(0, 128, 255, 255);
	glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, 0);
	
	glDepthMask(GL_TRUE);
	glDisable(GL_POLYGON_OFFSET_LINE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	
	glDisableClientState(GL_VERTEX_ARRAY);
}

void SceneModel::RenderGeometry::DrawNormals(float l)
{
	glLineWidth(2.f);
	
	glBegin(GL_LINES);
	glColor4ub(255, 128, 0, 255);
	for (VertexArray::const_iterator it = m_geometry.m_vertices.begin(), itEnd = m_geometry.m_vertices.end(); it != itEnd; ++it)
	{
		glVertex3fv(it->pos);
		glVertex3fv(it->pos + it->normal * l);
	}
	glEnd();
}

// ------------------------------------------------------------------------ //

