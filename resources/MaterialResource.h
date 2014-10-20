//
//  Material.h
//  MiRay/resources
//
//  Created by Damir Sagidullin on 21.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

#include "../rt/Material.h"

namespace mr
{

class MaterialManager;
class MaterialLayerImpl;

class MaterialResource : public IMaterial
{
	friend MaterialManager;
	MaterialManager & m_owner;

	const std::string m_name;
	std::vector<MaterialLayerImpl *>	m_layers;

	MaterialResource(MaterialManager & owner, const char * name);

public:
	virtual ~MaterialResource();

	void Create();

	void Load(pugi::xml_node node);
	void Save(pugi::xml_node node);

	void LoadTextures(ImageManager * pImageManager);

	const std::string & Name() const { return m_name; }

	size_t NumLayers() const;
	const IMaterialLayer * Layer(size_t i) const;
};

}