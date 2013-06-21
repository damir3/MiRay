//
//  RenderModel.h
//  MiRay/ui
//
//  Created by Damir Sagidullin on 14.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

namespace mr
{

class RenderModel
{
	Model & m_model;

	struct RenderGeometry
	{
		Model::Geometry & m_geometry;
		
		GLsizei	m_vertexCount;
		GLsizei	m_indexCount;

		GLuint	m_vertexBuffer;
		GLuint	m_indexBuffer;
		
		RenderGeometry(Model::Geometry & geom);
		~RenderGeometry();

		void Draw();
		void DrawWireframe();
		void DrawNormals(float l);
	};

	struct RenderMesh
	{
		Model::Mesh & m_mesh;

		typedef std::vector<RenderGeometry *>	RenderGeometryArray;
		RenderGeometryArray	m_geometries;

		RenderMesh(Model::Mesh & mesh);
		~RenderMesh();

		void Draw();
		void DrawWireframe();
		void DrawNormals(float l);
	};

	typedef std::vector<RenderMesh *>	RenderMeshArray;
	RenderMeshArray	m_meshes;

public:
	RenderModel(Model & model);
	~RenderModel();

	void Draw();
	void DrawWireframe();
	void DrawNormals(float l);
};

}
