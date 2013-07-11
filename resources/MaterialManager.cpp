//
//  MaterialManaget.cpp
//  MiRay/resources
//
//  Created by Damir Sagidullin on 21.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#include "MaterialManager.h"

using namespace mr;

// ------------------------------------------------------------------------ //

MaterialManager::MaterialManager()
{
}

MaterialManager::~MaterialManager()
{
}

// ------------------------------------------------------------------------ //

MaterialResource * MaterialManager::Get(const char * strName)
{
	return static_cast<MaterialResource *>(Find(strName));
	
}

MaterialResource * MaterialManager::Create(const char * strName)
{
	MaterialResource * pMaterial = static_cast<MaterialResource *>(Find(strName));
	if (pMaterial)
	{
		pMaterial->AddRef();
		return pMaterial;
	}
	
	printf("Creating Material '%s'...\n", strName);

	pMaterial = new MaterialResource(*this, strName);
	return pMaterial;
}

void MaterialManager::CleanupMaterials()
{
	for (Resources::const_iterator it = m_resources.begin(); it != m_resources.end(); ++it)
		(*it)->Release();
}

bool MaterialManager::LoadMaterials(const char *strFilename)
{
	pugi::xml_document doc;
	if (doc.load_file(strFilename).status != pugi::status_ok)
		return false;

	pugi::xml_node materials = doc.child("materials");
	for (pugi::xml_node material = materials.child("material"); material; material = material.next_sibling("material"))
	{
		const char * strName = material.attribute("name").value();
		if (!strName || !*strName)
			continue;

		MaterialResource * pMaterial = Create(strName);
		pMaterial->Load(material);
	}

	return true;
}

bool MaterialManager::SaveMaterials(const char * strFilename)
{
	pugi::xml_document doc;

	pugi::xml_node materials = doc.append_child("materials");
	for (Resources::const_iterator it = m_resources.begin(); it != m_resources.end(); ++it)
	{
		MaterialResource * pMaterial = static_cast<MaterialResource *>(*it);
		pugi::xml_node material = materials.append_child("material");
		material.append_attribute("name").set_value(pMaterial->Name());

		pMaterial->Save(material);
	}

	return doc.save_file(strFilename, "\t", pugi::format_default, pugi::encoding_utf8);
}

void MaterialManager::LoadTextures(ImageManager * pImageManager, const char * strLocalPath)
{
	for (Resources::const_iterator it = m_resources.begin(); it != m_resources.end(); ++it)
		static_cast<MaterialResource *>(*it)->LoadTextures(pImageManager, strLocalPath);
}
