//
//  IResource.cpp
//  MiRay/resources
//
//  Created by Damir Sagidullin on 22.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#include "IResource.h"

using namespace mr;

IResource * IResourceManager::Find(const char * strName) const
{
	for (Resources::const_iterator it = m_resources.begin(); it != m_resources.end(); ++it)
	{
		if (!strcmp((*it)->Name(), strName))
			return (*it);
	}
	
	return NULL;
}
