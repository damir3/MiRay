cmake_minimum_required(VERSION 3.28.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(LANGS CXX)
if(APPLE)
	list(APPEND LANGS OBJCXX)
endif()

project(MiRay VERSION 2.0.0.0 LANGUAGES ${LANGS})

# 1. Platform and Architecture detection (evaluated after project() so toolchains are loaded)
set(PLATFORM 64)

if(APPLE)
	if(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "arm|aarch64" OR CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm|aarch64" OR ARM)
		message(STATUS "Configuring for Apple Silicon Mac (arm64)")
		set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "OSX Architectures")
		set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum OS X deployment version")
		set(PLATFORM "arm")
	else()
		message(STATUS "Configuring for Intel Mac (x86_64)")
		set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "OSX Architectures")
		set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum OS X deployment version")
		set(PLATFORM "64")
	endif()
elseif(WIN32)
	if(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|arm64|aarch64" OR CMAKE_GENERATOR_PLATFORM MATCHES "ARM64|arm64" OR ARM64 OR ARM)
		message(STATUS "Configuring for Windows ARM64")
		set(PLATFORM "arm64")
	else()
		message(STATUS "Configuring for Windows x64")
		set(PLATFORM "64")
	endif()
endif()

set(APP_VER_MAJOR ${PROJECT_VERSION_MAJOR})
set(APP_VER_MINOR ${PROJECT_VERSION_MINOR})
set(APP_VER_BUILD ${PROJECT_VERSION_PATCH})
set(APP_VER_BETA 0)
set(APP_VER_STR "${APP_VER_MAJOR}.${APP_VER_MINOR}.${APP_VER_BUILD}")
string(TIMESTAMP VERSION_YEAR "%Y")

# 2. IDE Solution Properties
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "")
set_property(GLOBAL PROPERTY AUTOGEN_TARGETS_FOLDER "Generated")

# Source group helper for IDE file trees
function(miray_source_group)
	foreach(_source IN ITEMS ${ARGN})
		if(IS_ABSOLUTE "${_source}")
			file(RELATIVE_PATH _rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source}")
		else()
			set(_rel "${_source}")
		endif()
		if(NOT _rel MATCHES "^\\.\\.")
			get_filename_component(_path "${_rel}" DIRECTORY)
			if(_path)
				string(REPLACE "/" "\\" _group "${_path}")
				source_group("${_group}" FILES "${_source}")
			else()
				source_group("" FILES "${_source}")
			endif()
		endif()
	endforeach()
endfunction()

# 3. Compilation flags & defines
add_compile_definitions($<$<CONFIG:Debug>:_DEBUG>)

if(MSVC)
	add_compile_definitions(UNICODE _UNICODE NOMINMAX)
	add_compile_options(/we4715 /MP)
elseif(APPLE)
	add_compile_options(-fvisibility-inlines-hidden -fvisibility=hidden -Wno-reorder -Wno-switch)
	set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO")
	set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO")
	set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
	if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "15.0")
		add_link_options("-Wl,-no_warn_duplicate_libraries")
	endif()
endif()

# 4. Conan dependencies
find_package(Qt5 COMPONENTS Core Widgets Gui Network Qml Quick QuickWidgets OpenGL Xml Concurrent REQUIRED)
find_package(glm REQUIRED)
find_package(GTest REQUIRED)
find_package(freeimage REQUIRED)
find_package(assimp REQUIRED)
find_package(embree REQUIRED)
find_package(OpenImageDenoise REQUIRED)

# 5. Output directories
set(OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/bin/${PLATFORM})

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
	string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${OUTPUT_DIR})
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${OUTPUT_DIR})
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${OUTPUT_DIR})
endforeach()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# 6. Subprojects
add_subdirectory(Shared)
add_subdirectory(Core)
add_subdirectory(MiRay)
add_subdirectory(FormatModels3D)
add_subdirectory(Core.Tests)

# 7. Windows NSIS Installer Configuration
if(MSVC)
	if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tools/win/installer/miray.nsi.in")
		set(NSIS_PLATFORM ${PLATFORM})
		set(NSIS_VER_STR ${APP_VER_STR})
		configure_file(
			"${CMAKE_CURRENT_SOURCE_DIR}/tools/win/installer/miray.nsi.in"
			"${CMAKE_CURRENT_BINARY_DIR}/miray-${NSIS_PLATFORM}.nsi"
			@ONLY
		)
	endif()
endif()

# 8. Testing
enable_testing()
set_property(TARGET Core.Tests PROPERTY FOLDER "Tests")
