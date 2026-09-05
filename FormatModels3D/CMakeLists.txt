project(FormatModels3D)

set(FORMAT_3D_SOURCES
	FormatDescriptor.h
	SceneIR.h
	AssetResolver.h
	AssetResolver.cpp
	GeometryProcessor.h
	GeometryProcessor.cpp
	SceneBuilder.h
	SceneBuilder.cpp
	ModelLoaders.h
	ModelLoaders.cpp
	Plugin.h
	Plugin.cpp
	precompiled.h
)

if(MSVC)
	configure_file("${CMAKE_SOURCE_DIR}/tools/win/version.rc.in" "${CMAKE_CURRENT_BINARY_DIR}/version.rc" @ONLY)
	list(APPEND FORMAT_3D_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/version.rc")
endif()

add_library(FormatModels3D SHARED ${FORMAT_3D_SOURCES})

set_target_properties(FormatModels3D PROPERTIES
	AUTOMOC ON
	SUFFIX .plugin
)

# Native precompiled headers (CMake 3.16+)
target_precompile_headers(FormatModels3D PRIVATE precompiled.h)

target_link_libraries(FormatModels3D
	PRIVATE
		Shared
		glm::glm
		assimp::assimp
		Qt5::Core
		Qt5::Gui
		Qt5::Xml
)

if(MSVC)
	target_compile_definitions(FormatModels3D PRIVATE _CRT_SECURE_NO_WARNINGS)
elseif(APPLE)
	find_library(CORE_FOUNDATION CoreFoundation REQUIRED)
	find_library(APP_KIT AppKit REQUIRED)
	find_library(COCOA Cocoa REQUIRED)
	target_link_libraries(FormatModels3D PRIVATE ${CORE_FOUNDATION} ${APP_Kit} ${COCOA})
	target_compile_options(FormatModels3D PRIVATE
		-Wno-address-of-packed-member
		-Wno-nonportable-include-path
		-Wno-reorder
		-Wno-switch
	)
endif()

miray_source_group(${FORMAT_3D_SOURCES})
