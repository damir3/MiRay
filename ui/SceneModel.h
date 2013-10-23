//
//  SceneModel.h
//  MiRay/ui
//
//  Created by Damir Sagidullin on 25.08.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

namespace mr
{

class BVH;
class CollisionVolume;
class Model;

class SceneModel : public ITransformable
{
	BVH				&m_bvh;
	Model			*m_pModel;
	CollisionVolume *m_pVolume;

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
	SceneModel(BVH &bvh);
	~SceneModel();

	bool Init(const char *pFilename, const Matrix &mat, ModelManager *pModelManager, pugi::xml_node node);

	bool Load(pugi::xml_node node, ModelManager *pModelManager);
	void Save(pugi::xml_node node) const;
	
	BBoxF OOBB() const;
	Matrix Transformation() const;
	void SetTransformation(const Matrix & mat);

	CollisionVolume * GetVolume() const { return m_pVolume; }

	void Draw();
	void DrawWireframe();
	void DrawNormals(float l);
};
	
}