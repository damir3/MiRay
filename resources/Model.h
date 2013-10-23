//
//  Model.h
//  MiRay/resources
//
//  Created by Damir Sagidullin on 03.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

#include "IResource.h"

namespace mr
{

typedef std::vector<Vertex>		VertexArray;
typedef std::vector<uint32>		IndexArray;

class Image;
class MaterialResource;
class ModelManager;
class MaterialManager;

class Model : public IResource
{
public:
	struct Geometry
	{
		MaterialResource *m_pMaterial;
		VertexArray		m_vertices;
		IndexArray		m_indices;
		BBox			m_bbox;

		Geometry();
		~Geometry();
	};
	
	typedef std::vector<Geometry *>	GeometryArray;
	
	struct Mesh
	{
		GeometryArray	m_geometries;
		BBox			m_bbox;
		
		Mesh();
		~Mesh();
	};
	
	typedef std::vector<Mesh *>		MeshArray;
	
	struct Node
	{
		Matrix			m_matLocalTransform;
		Matrix			m_matWorldTransform;
		Mesh *			m_pMesh;
		
		const Matrix & LocalTransform() const { return m_matLocalTransform; }
	};
	
	const BBox & BoundingBox() const { return m_bbox; }
	const MeshArray & Meshes() const { return m_meshes; }
	MaterialManager * MaterialManagerPtr() const { return m_pMaterialManager; }

	void Save(pugi::xml_node node) const;

protected:
	friend ModelManager;
	
	Model(ModelManager & owner, const char * strName);
	~Model();
	
	BBox			m_bbox;
	MeshArray		m_meshes;
	MaterialManager	*m_pMaterialManager;
};
	
}