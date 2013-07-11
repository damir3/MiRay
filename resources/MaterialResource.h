//
//  Material.h
//  MiRay/resources
//
//  Created by Damir Sagidullin on 21.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

#include "IResource.h"
#include "../rt/Material.h"

namespace mr
{

class MaterialManager;
class MaterialLayerImpl;

class MaterialResource : public IResource, public IMaterial
{
	friend MaterialManager;

	MaterialResource(MaterialManager & owner, const char * strName);
	~MaterialResource();

	std::vector<MaterialLayerImpl *>	m_layers;

public:
	void Create();

	void Load(pugi::xml_node node);
	void Save(pugi::xml_node node);

	void LoadTextures(ImageManager * pImageManager, const char * strLocalPath);
	
	size_t NumLayers() const;
	const class MaterialLayer * Layer(size_t i) const;
};

}