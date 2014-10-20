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

MaterialPtr MaterialManager::Get(const char * strName)
{
	auto it = m_resources.find(strName);
	if (it == m_resources.end())
		return nullptr;

	return it->second.lock();
}

MaterialPtr MaterialManager::Create(const char * strName)
{
	auto it = m_resources.find(strName);
	if (it != m_resources.end())
	{
		MaterialPtr spMaterial = it->second.lock();
		if (spMaterial)
			return spMaterial;
	}

	printf("Creating Material '%s'...\n", strName);

	MaterialPtr spMaterial(new MaterialResource(*this, strName));
	m_resources[spMaterial->Name()] = spMaterial;

	return std::move(spMaterial);
}

void MaterialManager::Release(const std::string & name)
{
	auto it = m_resources.find(name);
	if (it != m_resources.end())
		m_resources.erase(it);
}

void MaterialManager::CleanupMaterials()
{
	m_vTmpMaterials.clear();
}

bool MaterialManager::LoadMaterials(pugi::xml_node doc)
{
	pugi::xml_node materials = doc.child("materials");
	for (pugi::xml_node material = materials.child("material"); material; material = material.next_sibling("material"))
	{
		const char * strName = material.attribute("name").value();
		if (!strName || !*strName)
			continue;

		MaterialPtr spMaterial = Create(strName);
		assert(spMaterial);

		spMaterial->Load(material);
		m_vTmpMaterials.push_back(spMaterial);
	}

	return true;
}

bool MaterialManager::SaveMaterials(pugi::xml_node doc)
{
	pugi::xml_node materials = doc.append_child("materials");
	for (auto it = m_resources.begin(); it != m_resources.end(); ++it)
	{
		MaterialPtr spMaterial = it->second.lock();
		if (spMaterial)
		{
			pugi::xml_node material = materials.append_child("material");
			material.append_attribute("name").set_value(spMaterial->Name().c_str());

			spMaterial->Save(material);
		}
	}
	return true;
}

void MaterialManager::LoadTextures(ImageManager * pImageManager)
{
	for (auto it = m_resources.begin(); it != m_resources.end(); ++it)
	{
		MaterialPtr spMaterial = it->second.lock();
		if (spMaterial)
			spMaterial->LoadTextures(pImageManager);
	}
}
