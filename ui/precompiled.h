#ifndef ui_precompiled_h
#define ui_precompiled_h

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define NOMINMAX
#include <windows.h>
#include <GL/glew.h>
#endif

#include <cassert>
#include <algorithm>
#include <string>
#include <vector>
#include <set>
#include <map>

#define FBXSDK_NEW_API
#define FBXSDK_SHARED
#include <fbxsdk.h>

#include "../ThirdParty/pugixml/pugixml.hpp"

#include "../common/all.h"
#include "../common/timer.h"
#include "../rt/BVH.h"
#include "../resources/ImageManager.h"
#include "../resources/MaterialManager.h"
#include "../resources/ModelManager.h"
#include "SceneInterfaces.h"

#ifdef __APPLE__
#include <OpenGL/GL.h>
#endif

#endif
