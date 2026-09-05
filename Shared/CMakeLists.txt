project(Shared)

set(SHARED_SOURCES
	glm-math.h
	Interfaces/ApplicationContext.h
	Interfaces/Camera.h
	Interfaces/CoreInstance.h
	Interfaces/Geometry.h
	Interfaces/Image.h
	Interfaces/LightNode.h
	Interfaces/Log.h
	Interfaces/Material.h
	Interfaces/MaterialManager.h
	Interfaces/MeshNode.h
	Interfaces/ModelLoader.h
	Interfaces/ModelSaver.h
	Interfaces/Node.h
	Interfaces/ObjectsFactory.h
	Interfaces/Parameters.h
	Interfaces/Plugin.h
	Interfaces/CoreRenderer.h
	Interfaces/RenderLayer.h
	Interfaces/RenderLayerManager.h
	Interfaces/Scene.h
	Interfaces/SceneElement.h
	Interfaces/SerializationContext.h
	Interfaces/Settings.h
	Interfaces/Snapshot.h
	Interfaces/SnapshotManager.h

	Utils/FileUtils.h
	Utils/Log.h
	Utils/MetaTypes.h
	Utils/FileUtils.cpp
	Utils/Log.cpp
)

if(MSVC)
	configure_file("${CMAKE_SOURCE_DIR}/tools/win/version.rc.in" "${CMAKE_CURRENT_BINARY_DIR}/version.rc" @ONLY)
	list(APPEND SHARED_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/version.rc")
endif()

add_library(Shared SHARED ${SHARED_SOURCES})

set_target_properties(Shared PROPERTIES AUTOMOC ON)

# Native precompiled headers (CMake 3.16+)
target_precompile_headers(Shared PRIVATE precompiled.h)

target_link_libraries(Shared
	PUBLIC
		glm::glm
		Qt5::Core
		Qt5::Gui
		Qt5::Widgets
)

target_include_directories(Shared
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(Shared
	INTERFACE SHAREDLIB_EXPORT=Q_DECL_IMPORT
	PRIVATE SHAREDLIB_EXPORT=Q_DECL_EXPORT
)

if(APPLE)
	target_compile_options(Shared PRIVATE -Wno-reorder)
endif()

miray_source_group(${SHARED_SOURCES})