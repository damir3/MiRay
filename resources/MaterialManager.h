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

class MaterialManager : public IResourceManager
{
public:
	MaterialManager();
	~MaterialManager();

	MaterialResource * Get(const char * strName);
	MaterialResource * Create(const char * strName);
	void CleanupMaterials();
	void LoadTextures(ImageManager * pImageManager, const char * strLocalPath);

	bool LoadMaterials(const char * strFilename);
	bool SaveMaterials(const char * strFilename);
};

}