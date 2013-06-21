//
//  MaterialManaget.cpp
//  MiRay/resources
//
//  Created by Damir Sagidullin on 21.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#include "MaterialManager.h"

#include "../ThirdParty/pugixml/pugixml.hpp"

using namespace mr;

// ------------------------------------------------------------------------ //

MaterialManager::MaterialManager()
{
}

MaterialManager::~MaterialManager()
{
}

// ------------------------------------------------------------------------ //

Material * MaterialManager::Get(const char * strName)
{
	return static_cast<Material *>(Find(strName));
	
}

Material * MaterialManager::Create(const char * strName)
{
	Material * pMaterial = static_cast<Material *>(Find(strName));
	if (pMaterial)
	{
		pMaterial->AddRef();
		return pMaterial;
	}
	
	printf("Creating Material '%s'...\n", strName);

	pMaterial = new Material(*this, strName);

	return pMaterial;
}

void MaterialManager::CleanupMaterials()
{
	for (Resources::const_iterator it = m_resources.begin(); it != m_resources.end(); ++it)
		(*it)->Release();
}

static void ColorFromString(ColorF & out, const char * str, const ColorF & def = ColorF::Null)
{
	if (!str || !*str || sscanf(str, "%f %f %f", &out.r, &out.g, &out.b) != 3)
	{
		out.r = def.r;
		out.g = def.g;
		out.b = def.b;
	}
}

static void FloatFromString(float & out, const char * str, float def = 0.f)
{
	if (!str || !*str || sscanf(str, "%f", &out) != 1)
		out = def;
}

static void BoolFromString(bool & out, const char * str, bool def = false)
{
	int i = 0;
	if (!str || !*str || sscanf(str, "%d", &i) != 1)
		out = def;

	out = (i != 0);
}

static const char * ColorToString(const ColorF & c, char * buf, size_t sz)
{
	snprintf(buf, sz, "%g %g %g", c.r, c.g, c.b);
	return buf;
}

static const char * FloatToString(float f, char * buf, size_t sz)
{
	snprintf(buf, sz, "%g", f);
	return buf;
}

static const char * BoolToString(bool b, char * buf, size_t sz)
{
	snprintf(buf, sz, "%d", b ? 1 : 0);
	return buf;
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

		Material * pMaterial = Create(strName);

		pugi::xml_node ambient = material.child("ambient");
		if (!ambient.empty())
		{
			ColorFromString(pMaterial->m_ambientColor, ambient.child("color").text().get());
			FloatFromString(pMaterial->m_ambientColor.a, ambient.child("level").text().get());
		}

		pugi::xml_node emissive = material.child("emissive");
		if (!emissive.empty())
		{
			ColorFromString(pMaterial->m_emissiveColor, emissive.child("color").text().get());
			FloatFromString(pMaterial->m_emissiveColor.a, emissive.child("level").text().get());
		}

		pugi::xml_node diffuse = material.child("diffuse");
		if (!diffuse.empty())
		{
			ColorFromString(pMaterial->m_diffuseColor, diffuse.child("color").text().get());
			pMaterial->m_diffuseTexName = diffuse.child("texture").text().get();
		}

		FloatFromString(pMaterial->m_opacity, material.child("opacity").text().get(), pMaterial->m_opacity);
		FloatFromString(pMaterial->m_ior, material.child("ior").text().get(), pMaterial->m_ior);

		pugi::xml_node reflection = material.child("reflection");
		if (!reflection.empty())
		{
			ColorFromString(pMaterial->m_reflectionColor, reflection.child("color").text().get());
			FloatFromString(pMaterial->m_reflectionColor.a, reflection.child("level").text().get());
			FloatFromString(pMaterial->m_reflectionGlossiness, reflection.child("glossiness").text().get(), pMaterial->m_reflectionGlossiness);
			BoolFromString(pMaterial->m_reflectionFresnel, reflection.child("fresnel").text().get(), pMaterial->m_reflectionFresnel);
		}

		pugi::xml_node refraction = material.child("refraction");
		if (!refraction.empty())
		{
			ColorFromString(pMaterial->m_refractionColor, refraction.child("color").text().get());
			FloatFromString(pMaterial->m_refractionColor.a, refraction.child("level").text().get());
			FloatFromString(pMaterial->m_refractionGlossiness, refraction.child("glossiness").text().get(), pMaterial->m_refractionGlossiness);
		}

		pugi::xml_node bump = material.child("bump");
		if (!bump.empty())
		{
			pMaterial->m_bumpMapName = bump.child("texture").text().get();
			FloatFromString(pMaterial->m_bumpLevel, bump.child("level").text().get());
		}
	}

	return true;
}

bool MaterialManager::SaveMaterials(const char * strFilename)
{
	pugi::xml_document doc;
//	doc.set_name("Materials");
//	doc.set_value("ABC");
	char buf[256];

	pugi::xml_node materials = doc.append_child("materials");
	for (Resources::const_iterator it = m_resources.begin(); it != m_resources.end(); ++it)
	{
		Material * pMaterial = static_cast<Material *>(*it);
		pugi::xml_node material = materials.append_child("material");
		material.append_attribute("name").set_value(pMaterial->Name());

		if (pMaterial->m_ambientColor != ColorF::Null)
		{
			pugi::xml_node ambient = material.append_child("ambient");
			ambient.append_child("color").text().set(ColorToString(pMaterial->m_ambientColor, buf, sizeof(buf)));
			ambient.append_child("level").text().set(FloatToString(pMaterial->m_ambientColor.a, buf, sizeof(buf)));
		}

		if (pMaterial->m_emissiveColor != ColorF::Null)
		{
			pugi::xml_node emissive = material.append_child("emissive");
			emissive.append_child("color").text().set(ColorToString(pMaterial->m_emissiveColor, buf, sizeof(buf)));
			emissive.append_child("level").text().set(FloatToString(pMaterial->m_emissiveColor.a, buf, sizeof(buf)));
		}

		pugi::xml_node diffuse = material.append_child("diffuse");
		diffuse.append_child("color").text().set(ColorToString(pMaterial->m_diffuseColor, buf, sizeof(buf)));
		if (!pMaterial->m_diffuseTexName.empty())
			diffuse.append_child("texture").text().set(pMaterial->m_diffuseTexName.c_str());

		material.append_child("opacity").text().set(FloatToString(pMaterial->m_opacity, buf, sizeof(buf)));
		material.append_child("ior").text().set(FloatToString(pMaterial->m_ior, buf, sizeof(buf)));

		pugi::xml_node reflection = material.append_child("reflection");
		reflection.append_child("color").text().set(ColorToString(pMaterial->m_reflectionColor, buf, sizeof(buf)));
		reflection.append_child("level").text().set(FloatToString(pMaterial->m_reflectionColor.a, buf, sizeof(buf)));
		reflection.append_child("glossiness").text().set(FloatToString(pMaterial->m_reflectionGlossiness, buf, sizeof(buf)));
		reflection.append_child("fresnel").text().set(BoolToString(pMaterial->m_reflectionFresnel, buf, sizeof(buf)));

		pugi::xml_node refraction = material.append_child("refraction");
		refraction.append_child("color").text().set(ColorToString(pMaterial->m_refractionColor, buf, sizeof(buf)));
		refraction.append_child("glossiness").text().set(FloatToString(pMaterial->m_refractionGlossiness, buf, sizeof(buf)));

		if (!pMaterial->m_bumpMapName.empty())
		{
			pugi::xml_node bump = material.append_child("bump");
			bump.append_child("texture").text().set(pMaterial->m_bumpMapName.c_str());
			bump.append_child("level").text().set(FloatToString(pMaterial->m_bumpLevel, buf, sizeof(buf)));
		}
	}

	return doc.save_file(strFilename, "\t", pugi::format_default, pugi::encoding_utf8);
}

//static std::string RealFileName(const std::string & strFilename, const char * strLocalPath)
//{
//	std::string strLocalFilename = strFilename;
//	size_t p = strLocalFilename.find_last_of('/');
//	if (p != std::string::npos)
//		strLocalFilename.erase(0, p);
//
//	p = strLocalFilename.find_last_of('\\');
//	if (p != std::string::npos)
//		strLocalFilename.erase(0, p);
//
//	return !strLocalFilename.empty() ? (strLocalPath + strLocalFilename) : "";
//}

void MaterialManager::LoadTextures(ImageManager * pImageManager, const char * strLocalPath)
{
	for (Resources::const_iterator it = m_resources.begin(); it != m_resources.end(); ++it)
	{
		Material * pMaterial = static_cast<Material *>(*it);

		if (!pMaterial->m_diffuseTexName.empty())
			pMaterial->m_pDiffuseTexture = pImageManager->Load((strLocalPath + pMaterial->m_diffuseTexName).c_str());

		if (!pMaterial->m_bumpMapName.empty())
			pMaterial->m_pNormalmap = pImageManager->LoadNormalmap((strLocalPath + pMaterial->m_bumpMapName).c_str());
	}
}
