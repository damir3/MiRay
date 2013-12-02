//
//  MaterialManaget.h
//  MiRay/resources
//
//  Created by Damir Sagidullin on 21.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

#include "MaterialResource.h"

namespace mr
{

typedef std::shared_ptr<MaterialResource> MaterialPtr;

class MaterialManager
{
	std::map<std::string, std::weak_ptr<MaterialResource>> m_resources;
	std::vector<MaterialPtr> m_vTmpMaterials;

	friend MaterialResource;
	void Release(const std::string & name);

public:
	MaterialManager();
	~MaterialManager();

	MaterialPtr Get(const char * strName);
	MaterialPtr Create(const char * strName);

	void CleanupMaterials();
	void LoadTextures(ImageManager * pImageManager);

	bool LoadMaterials(pugi::xml_node node);
	bool SaveMaterials(pugi::xml_node node);
};

}