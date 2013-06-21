//
//  precompiled.h
//  MiRay/MiRay Studio/win
//
//  Created by Damir Sagidullin on 17.06.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <tchar.h>
#include <assert.h>
#include <cstdlib>
#include <algorithm>
#include <string>
#include <vector>
#include <set>

#include <gl/glew.h>

#include "../../common/all.h"
#include "../../ui/SceneView.h"
