//
//  precompiled.h
//  MiRay/resources
//
//  Created by Damir Sagidullin on 01.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#ifndef miray_resources_precompiled_h
#define miray_resources_precompiled_h

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define NOMINMAX
#include <windows.h>
#endif

#include <cassert>
#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <sstream>

#define FBXSDK_NEW_API
#define FBXSDK_SHARED
#include <fbxsdk.h>

#include "../ThirdParty/pugixml/pugixml.hpp"

#include "../common/all.h"
#include "ImageManager.h"
#include "MaterialManager.h"
#include "ModelManager.h"

#endif
