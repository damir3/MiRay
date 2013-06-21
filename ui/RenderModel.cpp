//
//  RenderModel.cpp
//  MiRay/ui
//
//  Created by Damir Sagidullin on 14.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#include "RenderModel.h"

using namespace mr;

RenderModel::RenderModel(Model & model)
	: m_model(model)
{
	m_meshes.reserve(model.Meshes().size());
	for (size_t i = 0; i < model.Meshes().size(); i++)
		m_meshes.push_back(new RenderMesh(*model.Meshes()[i]));
}

RenderModel::~RenderModel()
{
	while (!m_meshes.empty())
	{
		delete m_meshes.back();
		m_meshes.pop_back();
	}
}

void RenderModel::Draw()
{
	for (RenderMeshArray::iterator it = m_meshes.begin(), itEnd = m_meshes.end(); it != itEnd; ++it)
		(*it)->Draw();
}

void RenderModel::DrawWireframe()
{
	for (RenderMeshArray::iterator it = m_meshes.begin(), itEnd = m_meshes.end(); it != itEnd; ++it)
		(*it)->DrawWireframe();
}

void RenderModel::DrawNormals(float l)
{
	for (RenderMeshArray::iterator it = m_meshes.begin(), itEnd = m_meshes.end(); it != itEnd; ++it)
		(*it)->DrawNormals(l);
}

// ------------------------------------------------------------------------ //

RenderModel::RenderMesh::RenderMesh(Model::Mesh & mesh)
	: m_mesh(mesh)
{
	m_geometries.reserve(mesh.m_geometries.size());
	for (size_t i = 0; i < mesh.m_geometries.size(); i++)
		m_geometries.push_back(new RenderGeometry(*mesh.m_geometries[i]));
}

RenderModel::RenderMesh::~RenderMesh()
{
	while (!m_geometries.empty())
	{
		delete m_geometries.back();
		m_geometries.pop_back();
	}
}

void RenderModel::RenderMesh::Draw()
{
	for (RenderGeometryArray::iterator it = m_geometries.begin(), itEnd = m_geometries.end(); it != itEnd; ++it)
		(*it)->Draw();
}

void RenderModel::RenderMesh::DrawWireframe()
{
	for (RenderGeometryArray::iterator it = m_geometries.begin(), itEnd = m_geometries.end(); it != itEnd; ++it)
		(*it)->DrawWireframe();
}

void RenderModel::RenderMesh::DrawNormals(float l)
{
	for (RenderGeometryArray::iterator it = m_geometries.begin(), itEnd = m_geometries.end(); it != itEnd; ++it)
		(*it)->DrawNormals(l);
}

// ------------------------------------------------------------------------ //

struct sArrayBufferVertex
{
	Vec3 pos;
	Color color;
};

RenderModel::RenderGeometry::RenderGeometry(Model::Geometry & geom)
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

RenderModel::RenderGeometry::~RenderGeometry()
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

void RenderModel::RenderGeometry::Draw()
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
	
void RenderModel::RenderGeometry::DrawWireframe()
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

void RenderModel::RenderGeometry::DrawNormals(float l)
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

