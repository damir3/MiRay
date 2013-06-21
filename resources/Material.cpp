//
//  Material.cpp
//  MiRay/resources
//
//  Created by Damir Sagidullin on 21.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#include "Material.h"

using namespace mr;

Material::Material(MaterialManager & owner, const char * strName)
	: IResource(owner, strName)
	, m_ambientColor(ColorF::Null)
	, m_emissiveColor(ColorF::Null)
	, m_diffuseColor(ColorF::White)
	, m_pDiffuseTexture(NULL)
	, m_opacity(1.f)
	, m_ior(1.5f)
	, m_reflectionColor(1.f, 1.f, 1.f, 0.f)
	, m_reflectionFresnel(false)
	, m_reflectionGlossiness(1.f)
	, m_reflectionExitColor(ColorF::Null)
	, m_refractionColor(1.f, 1.f, 1.f, 0.f)
	, m_refractionGlossiness(1.f)
	, m_refractionExitColor(ColorF::Null)
	, m_bumpLevel(0.5f)
	, m_pNormalmap(NULL)
{
}

Material::~Material()
{
	SAFE_RELEASE(m_pDiffuseTexture);
}
