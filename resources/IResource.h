//
//  IResource.h
//  MiRay/resources
//
//  Created by Damir Sagidullin on 22.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

namespace mr
{

class IResource;

class IResourceManager
{
protected:
	friend IResource;
	typedef std::set<IResource *>	Resources;
	Resources	m_resources;

	IResourceManager() {}

	IResource * Find(const char * strName) const;

public:
	virtual ~IResourceManager()
	{
		assert(m_resources.empty());
	}
};

class IResource : public BaseRC
{
	friend IResourceManager;
	class IResourceManager & m_owner;
	const std::string m_strName;
	
protected:
	IResource(IResourceManager & owner, const char * strName) : m_owner(owner), m_strName(strName)
	{
		m_owner.m_resources.insert(this);
	}
	
public:
	virtual ~IResource()
	{
		m_owner.m_resources.erase(this);
	}
	
	const char * Name() const { return m_strName.c_str(); }
};

}
