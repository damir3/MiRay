//
//  Model.cpp
//  MiRay/resources
//
//  Created by Damir Sagidullin on 09.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#include "Model.h"

using namespace mr;

Model::Model(ModelManager & owner, const char * name)
	: m_owner(owner)
	, m_name(name)
	, m_pMaterialManager(new MaterialManager())
{
	m_bbox.ClearBounds();
}

Model::~Model()
{
	while (m_meshes.size() > 0)
	{
		delete m_meshes.back();
		m_meshes.pop_back();
	}

	m_owner.Release(m_name);
}

// ------------------------------------------------------------------------ //

void Model::Save(pugi::xml_node node) const
{
	m_pMaterialManager->SaveMaterials(node);
}

// ------------------------------------------------------------------------ //

Model::Mesh::Mesh()
{
	m_bbox.ClearBounds();
}

Model::Mesh::~Mesh()
{
	while (m_geometries.size() > 0)
	{
		delete m_geometries.back();
		m_geometries.pop_back();
	}
}

// ------------------------------------------------------------------------ //

Model::Geometry::Geometry() : m_pMaterial(NULL)
{
	m_bbox.ClearBounds();
}

Model::Geometry::~Geometry()
{
}
