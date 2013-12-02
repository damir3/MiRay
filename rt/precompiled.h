//
//  precompiled.h
//  MiRay
//
//  Created by Damir Sagidullin on 01.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#ifndef miray_rt_precompiled_h
#define miray_rt_precompiled_h

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define NOMINMAX
#include <windows.h>
#endif

#include <cassert>
#include <cmath>
#include <cfloat>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include "../common/all.h"
#include "../resources/Image.h"

#include "BVH.h"

#endif
