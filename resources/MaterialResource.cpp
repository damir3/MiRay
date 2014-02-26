//
//  node.cpp
//  MiRay/resources
//
//  Created by Damir Sagidullin on 21.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#include "MaterialResource.h"

namespace mr
{

class MaterialParameter
{
	Vec3			m_color;
	std::string		m_filename;
	ImagePtr		m_pTexture;

public:
	MaterialParameter(const Vec3 & c) : m_color(c) {}

	void SetColor(const Vec3 & c) { m_color = c; }
	const Vec3 & GetColor() const { return m_color; }
	
	void SetFilename(const char * filename) { m_filename = filename; }
	const std::string & Filename() const { return m_filename; }

	void SetTexture(ImagePtr &pTexture) { m_pTexture = pTexture; }
	bool HasTexture() const { return NULL != m_pTexture; }
	
	inline float Value(const Vec2 & tc) const
	{
		if (!m_pTexture)
			return m_color.x;
		
		return m_pTexture->GetPixel(tc.x, tc.y).r * m_color.x;
	}
	
	inline Vec3 Color(const Vec2 & tc) const
	{
		if (!m_pTexture)
			return m_color;
		
		return Vec3(m_pTexture->GetPixel(tc.x, tc.y)) * m_color;
	}
};
	
// ------------------------------------------------------------------------ //

class MaterialLayerImpl : public IMaterialLayer
{
	friend MaterialManager;

	MaterialParameter	m_ambient;
	MaterialParameter	m_emissive;
	MaterialParameter	m_diffuse;
	MaterialParameter	m_opacity;
	MaterialParameter	m_indexOfRefraction;
	
	bool m_fresnelReflection;
	MaterialParameter	m_reflection;
	MaterialParameter	m_reflectionTint;
	MaterialParameter	m_reflectionRoughness;
	MaterialParameter	m_reflectionExitColor;
	
	MaterialParameter	m_refractionTint;
	MaterialParameter	m_refractionRoughness;
	MaterialParameter	m_refractionExitColor;
	
	MaterialParameter	m_bumpLevel;
	MaterialParameter	m_normalmap;
	
	std::string		m_bumpMapName;
	ImagePtr		m_pNormalmap;

public:
	MaterialLayerImpl();
	~MaterialLayerImpl();

	Vec3 Ambient(const sMaterialContext & mc) const { return m_ambient.Color(mc.tc); }
	Vec3 Emissive(const sMaterialContext & mc) const { return m_emissive.Color(mc.tc); }
	Vec3 Diffuse(const sMaterialContext & mc) const { return m_diffuse.Color(mc.tc); }
	Vec3 Opacity(const sMaterialContext & mc) const { return m_opacity.Color(mc.tc); }
	Vec3 IndexOfRefraction(const sMaterialContext & mc) const { return m_indexOfRefraction.Color(mc.tc); }
	
	bool FresnelReflection() const { return m_fresnelReflection; }
	bool RaytracedReflection() const { return true; }
	
	Vec3 Reflection(const sMaterialContext & mc) const { return m_reflection.Color(mc.tc); }
	Vec3 ReflectionTint(const sMaterialContext & mc) const { return m_reflectionTint.Color(mc.tc); }
	float ReflectionRoughness(const sMaterialContext & mc) const { return m_reflectionTint.Value(mc.tc); }
	Vec3 ReflectionExitColor(const sMaterialContext & mc) const { return m_reflectionExitColor.Color(mc.tc); }
	bool HasReflectionMap() const { return false; }
	Vec3 ReflectionMap(const sMaterialContext & mc) const { return Vec3::Null; }
	
	Vec3 RefractionTint(const sMaterialContext & mc) const { return m_refractionTint.Color(mc.tc); }
	float RefractionRoughness(const sMaterialContext & mc) const { return m_refractionRoughness.Value(mc.tc); }
	Vec3 RefractionExitColor(const sMaterialContext & mc) const { return m_refractionExitColor.Color(mc.tc); }
	
	bool HasNormalMap() const { return m_normalmap.HasTexture(); }
	Vec3 NormalMap(const sMaterialContext & mc) const
	{
		mr::Vec3 nm = m_normalmap.Color(mc.tc) * 2.f - mr::Vec3(1.f);
		return mr::Vec3::Lerp(mr::Vec3::Z, nm, m_bumpLevel.Value(mc.tc));
	}

	void LoadTextures(ImageManager * pImageManager);

	void Load(pugi::xml_node node);
	void Save(pugi::xml_node node);
};

}

using namespace mr;

// ------------------------------------------------------------------------ //

MaterialLayerImpl::MaterialLayerImpl()
	: m_ambient(0.f)
	, m_emissive(0.f)
	, m_diffuse(1.f)
	, m_opacity(1.f)
	, m_indexOfRefraction(1.5f)
	, m_fresnelReflection(false)
	, m_reflection(0.f)
	, m_reflectionTint(1.f)
	, m_reflectionRoughness(0.f)
	, m_reflectionExitColor(0.f)
	, m_refractionTint(1.f)
	, m_refractionRoughness(0.f)
	, m_refractionExitColor(0.f)
	, m_bumpLevel(1.f)
	, m_normalmap(1.f)
{
}

MaterialLayerImpl::~MaterialLayerImpl()
{
}

// ------------------------------------------------------------------------ //

static Vec3 ColorFromString(const char * str, const Vec3 & def = Vec3::Null)
{
	if (!str || !*str)
		return def;
	
	Vec3 out;
	int n = sscanf(str, "%f %f %f", &out.x, &out.y, &out.z);
	if (n == 1)
		out.y = out.z = out.x;
	else if (n != 3)
		return def;
	return out;
}

static float FloatFromString(const char * str, float def = 0.f)
{
	float out;
	if (!str || !*str || sscanf(str, "%f", &out) != 1)
		return def;
	return out;
}

static void BoolFromString(bool & out, const char * str, bool def = false)
{
	int i = 0;
	if (!str || !*str || sscanf(str, "%d", &i) != 1)
		out = def;
	
	out = (i != 0);
}

bool ReadMaterialParam(MaterialParameter & param, pugi::xml_node node, const char * name)
{
	pugi::xml_node value = node.child(name);
	if (!value.empty())
		param.SetColor(ColorFromString(value.text().get(), param.GetColor()));

	char map_name[64];
	strcpy(map_name, name);
	strcat(map_name, "-map");
	pugi::xml_node map = node.child(map_name);
	if (!map.empty())
		param.SetFilename(map.text().get());

	return true;
}

// ------------------------------------------------------------------------ //

static const char * ColorToString(const Vec3 & c, char * buf, size_t sz)
{
	if (c.x == c.y && c.x == c.z)
		snprintf(buf, sz, "%g", c.x);
	else
		snprintf(buf, sz, "%g %g %g", c.x, c.y, c.z);
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

void SaveMaterialParam(pugi::xml_node node, const char * name, const MaterialParameter & param)
{
	char buf[256];
	node.append_child(name).text().set(ColorToString(param.GetColor(), buf, sizeof(buf)));
	if (!param.Filename().empty())
	{
		strcpy(buf, name);
		strcat(buf, "-map");
		node.append_child(buf).text().set(param.Filename().c_str());
	}
}

// ------------------------------------------------------------------------ //

void MaterialLayerImpl::Load(pugi::xml_node node)
{
	ReadMaterialParam(m_ambient, node, "ambient");
	ReadMaterialParam(m_emissive, node, "emissive");
	ReadMaterialParam(m_diffuse, node, "diffuse");
	ReadMaterialParam(m_opacity, node, "opacity");
	
	ReadMaterialParam(m_indexOfRefraction, node, "ior");
	
	ReadMaterialParam(m_reflection, node, "reflection");
	ReadMaterialParam(m_reflectionTint, node, "reflection-tint");
	ReadMaterialParam(m_reflectionRoughness, node, "reflection-roughness");
	
	ReadMaterialParam(m_refractionTint, node, "refraction-tint");
	ReadMaterialParam(m_refractionRoughness, node, "refraction-roughness");
	
	BoolFromString(m_fresnelReflection, node.child("fresnel-reflection").text().get(), m_fresnelReflection);

	pugi::xml_node bump = node.child("bump-map");
	if (!bump.empty())
		m_bumpMapName = bump.text().get();

	ReadMaterialParam(m_bumpLevel, node, "bump-level");
}

void MaterialLayerImpl::Save(pugi::xml_node node)
{
	if (!m_ambient.Filename().empty() || (m_ambient.GetColor() != Vec3::Null))
		SaveMaterialParam(node, "ambient", m_ambient);

	if (!m_emissive.Filename().empty() || (m_emissive.GetColor() != Vec3::Null))
		SaveMaterialParam(node, "emissive", m_emissive);

	SaveMaterialParam(node, "diffuse", m_diffuse);
	
	SaveMaterialParam(node, "opacity", m_opacity);
	
	SaveMaterialParam(node, "ior", m_indexOfRefraction);
	
	char buf[256];
	node.append_child("fresnel-reflection").text().set(BoolToString(FresnelReflection(), buf, sizeof(buf)));

	if (!m_reflection.Filename().empty() || (m_reflection.GetColor() != Vec3::Null))
		SaveMaterialParam(node, "reflection", m_reflection);

	if (!m_reflectionTint.Filename().empty() || (m_reflectionTint.GetColor() != Vec3(1.f)))
		SaveMaterialParam(node, "reflection-tint", m_reflectionTint);

	if (!m_reflectionRoughness.Filename().empty() || (m_reflectionRoughness.GetColor() != Vec3::Null))
		SaveMaterialParam(node, "reflection-roughness", m_reflectionRoughness);

	if (!m_reflectionExitColor.Filename().empty() || (m_reflectionExitColor.GetColor() != Vec3::Null))
		SaveMaterialParam(node, "reflection-exit-color", m_reflectionExitColor);

	if (!m_refractionTint.Filename().empty() || (m_refractionTint.GetColor() != Vec3(1.f)))
		SaveMaterialParam(node, "refraction-tint", m_refractionTint);

	if (!m_refractionRoughness.Filename().empty() || (m_refractionRoughness.GetColor() != Vec3::Null))
		SaveMaterialParam(node, "refraction-roughness", m_refractionRoughness);

	if (!m_refractionExitColor.Filename().empty() || (m_refractionExitColor.GetColor() != Vec3::Null))
		SaveMaterialParam(node, "refraction-exit-color", m_refractionExitColor);

	if (!m_bumpMapName.empty())
	{
		node.append_child("bump-map").text().set(m_bumpMapName.c_str());

		if (!m_bumpLevel.Filename().empty() || (m_bumpLevel.GetColor() != Vec3::Null))
			SaveMaterialParam(node, "bump-level", m_bumpLevel);
	}
}

// ------------------------------------------------------------------------ //

void LoadTexture(MaterialParameter & texture, ImageManager * pImageManager)
{
	if (texture.Filename().empty())
		return;

	ImagePtr pTexture = pImageManager->Load(texture.Filename().c_str());
	if (!pTexture)
		return;

	texture.SetTexture(pTexture);
}

void MaterialLayerImpl::LoadTextures(ImageManager * pImageManager)
{
	LoadTexture(m_ambient, pImageManager);
	LoadTexture(m_emissive, pImageManager);
	LoadTexture(m_diffuse, pImageManager);
	LoadTexture(m_opacity, pImageManager);
	LoadTexture(m_indexOfRefraction, pImageManager);
	LoadTexture(m_reflection, pImageManager);
	LoadTexture(m_reflectionTint, pImageManager);
	LoadTexture(m_reflectionRoughness, pImageManager);
	LoadTexture(m_reflectionExitColor, pImageManager);
	LoadTexture(m_refractionTint, pImageManager);
	LoadTexture(m_refractionRoughness, pImageManager);
	LoadTexture(m_refractionExitColor, pImageManager);
	LoadTexture(m_bumpLevel, pImageManager);

	if (!m_bumpMapName.empty())
		m_pNormalmap = pImageManager->LoadNormalmap(m_bumpMapName.c_str());
}

// ------------------------------------------------------------------------ //

MaterialResource::MaterialResource(MaterialManager & owner, const char * name) : m_owner(owner), m_name(name)
{
}

MaterialResource::~MaterialResource()
{
	while (m_layers.empty())
	{
		delete m_layers.back();
		m_layers.pop_back();
	}

	m_owner.Release(m_name);
}

void MaterialResource::Create()
{
	m_layers.push_back(new MaterialLayerImpl());
}

void MaterialResource::Load(pugi::xml_node node)
{
	for (pugi::xml_node layer = node.child("layer"); layer; layer = layer.next_sibling("layer"))
	{
		m_layers.push_back(new MaterialLayerImpl());
		m_layers.back()->Load(layer);
	}

	if (m_layers.empty())
		m_layers.push_back(new MaterialLayerImpl());
}

void MaterialResource::Save(pugi::xml_node node)
{
	for (auto it = m_layers.begin(); it != m_layers.end(); ++it)
		(*it)->Save(node.append_child("layer"));
}

void MaterialResource::LoadTextures(ImageManager * pImageManager)
{
	for (auto it = m_layers.begin(); it != m_layers.end(); ++it)
		(*it)->LoadTextures(pImageManager);
}

size_t MaterialResource::NumLayers() const
{
	return m_layers.size();
}

const IMaterialLayer * MaterialResource::Layer(size_t i) const
{
	return i < m_layers.size() ? m_layers[i] : nullptr;
}

// ------------------------------------------------------------------------ //
