//
//  Material.h
//  MiRay/resources
//
//  Created by Damir Sagidullin on 21.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

#include "IResource.h"

namespace mr
{

class Image;
class MaterialManager;

class Material : public IResource
{
	friend MaterialManager;

	Material(MaterialManager & owner, const char * strName);
	~Material();

public:
	ColorF			m_ambientColor;

	ColorF			m_emissiveColor;

	ColorF			m_diffuseColor;
	std::string		m_diffuseTexName;
	Image			*m_pDiffuseTexture;

	std::string		m_specularTexName;
//	std::string		m_opacityTexName;

	float			m_opacity;
	float			m_ior;

	bool			m_reflectionFresnel;
	ColorF			m_reflectionColor;
	float			m_reflectionGlossiness;
	ColorF			m_reflectionExitColor;

	ColorF			m_refractionColor;
	float			m_refractionGlossiness;
	ColorF			m_refractionExitColor;

	std::string		m_bumpMapName;
	float			m_bumpLevel;
	Image			*m_pNormalmap;
};

}