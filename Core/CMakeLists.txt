project(Core)

set(CORE_SOURCES
	Application.h
	PluginManager.h
	src/AbstractLight.h
	src/Application.cpp
	src/AppLog.cpp
	src/AppLog.h
	src/Camera.cpp
	src/Camera.h
	src/CameraProjector.cpp
	src/CameraProjector.h
	src/ColorUtils.h
	src/ColorUtils.cpp
	src/skcms/skcms.h
	src/skcms/skcms.cc
	src/skcms/src/skcms_TransformBaseline.cc
	src/CoreInstance.cpp
	src/CoreInstance.h
	src/DirectionalLight.cpp
	src/DirectionalLight.h
	src/EnvironmentLight.cpp
	src/EnvironmentLight.h
	src/Geometry.cpp
	src/Geometry.h
	src/Gizmo.cpp
	src/Gizmo.h
	src/ImageImpl.cpp
	src/ImageImpl.h
	src/ImageManager.cpp
	src/ImageManager.h
	src/LightNode.cpp
	src/LightNode.h
	src/MainFactory.cpp
	src/MainFactory.h
	src/MeshNode.cpp
	src/MeshNode.h
	src/MiRaySceneLoader.cpp
	src/MiRaySceneLoader.h
	src/MiRaySceneSaver.cpp
	src/MiRaySceneSaver.h
	src/ModelLoadersRegistry.cpp
	src/ModelLoadersRegistry.h
	src/ModelSaversRegistry.cpp
	src/ModelSaversRegistry.h
	src/Node.cpp
	src/Node.h
	src/NodeUtils.cpp
	src/NodeUtils.h
	src/ParametersImpl.h
	src/Parameters/ColorParameterImpl.h
	src/Parameters/ColorParameterImpl.cpp
	src/Parameters/ScalarParameterImpl.h
	src/Parameters/ScalarParameterImpl.cpp
	src/Parameters/BooleanParameterImpl.h
	src/Parameters/BooleanParameterImpl.cpp
	src/Parameters/IntegerParameterImpl.h
	src/Parameters/IntegerParameterImpl.cpp
	src/Parameters/Vec2ParameterImpl.h
	src/Parameters/Vec2ParameterImpl.cpp
	src/Parameters/Vec3ParameterImpl.h
	src/Parameters/Vec3ParameterImpl.cpp
	src/Parameters/EnumParameterImpl.h
	src/Parameters/EnumParameterImpl.cpp
	src/Parameters/StringParameterImpl.h
	src/Parameters/StringParameterImpl.cpp
	src/Parameters/FileNameParameterImpl.h
	src/Parameters/FileNameParameterImpl.cpp
	src/PluginManager.cpp
	src/PhaseFunction.h
	src/precompiled.h
	src/Ray.cpp
	src/Ray.h
	src/CoreRenderer.cpp
	src/CoreRenderer.h
	src/RenderLayerImpl.cpp
	src/RenderLayerImpl.h
	src/RenderLayerManager.cpp
	src/RenderLayerManager.h
	src/RenderMath.cpp
	src/RenderMath.h
	src/RenderUtils.cpp
	src/RenderUtils.h
	src/Scene.cpp
	src/Scene.h
	src/SceneCommands.cpp
	src/SceneCommands.h
	src/SceneMergeMeshes.cpp
	src/Settings.cpp
	src/Settings.h
	src/SnapshotInterpolator.cpp
	src/SnapshotInterpolator.h
	src/SnapshotImpl.cpp
	src/SnapshotImpl.h
	src/SnapshotManager.cpp
	src/SnapshotManager.h
	src/TextureImpl.cpp
	src/TextureImpl.h
	src/TextureParametersImpl.cpp
	src/TextureParametersImpl.h
	src/Utils.cpp
	src/Utils.h
	src/Materials/BSDF.cpp
	src/Materials/BSDF.h
	src/Materials/IndexOfRefractionImpl.cpp
	src/Materials/IndexOfRefractionImpl.h
	src/Materials/IORReader.cpp
	src/Materials/IORReader.h
	src/Materials/MaterialGroupImpl.cpp
	src/Materials/MaterialGroupImpl.h
	src/Materials/MaterialImpl.cpp
	src/Materials/MaterialImpl.h
	src/Materials/MaterialLayerImpl.cpp
	src/Materials/MaterialLayerImpl.h
	src/Materials/MaterialManager.cpp
	src/Materials/MaterialManager.h
	src/Materials/Microfacet.cpp
	src/Materials/Microfacet.h
	src/Renderers/BaseRenderer.cpp
	src/Renderers/BaseRenderer.h
	src/Renderers/DummyRenderer.cpp
	src/Renderers/DummyRenderer.h
	src/Renderers/PriorityStack.h
	src/Renderers/StaticArray.h
	src/Renderers/VolumePathTracer.cpp
	src/Renderers/VolumePathTracer.h
	src/Renderers/UPBP/Bre.cpp
	src/Renderers/UPBP/Bre.h
	src/Renderers/UPBP/Defs.h
	src/Renderers/UPBP/Grid.h
	src/Renderers/UPBP/HashGrid.h
	src/Renderers/UPBP/KdTmpl.h
	src/Renderers/UPBP/PathWeight.h
	src/Renderers/UPBP/PhBeams.cpp
	src/Renderers/UPBP/PhBeams.h
	src/Renderers/UPBP/PhGrid.h
	src/Renderers/UPBP/PhotonBeam.h
	src/Renderers/UPBP/UPBP.cpp
	src/Renderers/UPBP/UPBP.h
	src/Renderers/UPBP/UPBPLightVertex.h
	src/Samplers/Halton.cpp
	src/Samplers/Halton.h
	src/Samplers/lowdiscrepancy.cpp
	src/Samplers/lowdiscrepancy.h
	src/Samplers/Random.h
	src/Samplers/Sampler.h
	src/Samplers/Sobol.cpp
	src/Samplers/Sobol.h
	src/Samplers/sobolmatrices.cpp
	src/Samplers/sobolmatrices.h
	src/Tools/EditNormals.cpp
	src/Tools/EditNormals.h
	src/Tools/FitToView.cpp
	src/Tools/FitToView.h
	src/Tools/PivotParameters.cpp
	src/Tools/PivotParameters.h
	src/Tools/UVMapping.cpp
	src/Tools/UVMapping.h
)

if(MSVC)
	configure_file("${CMAKE_SOURCE_DIR}/tools/win/version.rc.in" "${CMAKE_CURRENT_BINARY_DIR}/version.rc" @ONLY)
	list(APPEND CORE_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/version.rc")
endif()

qt5_add_resources(CORE_QRC_SOURCES res/resources.qrc)
list(APPEND CORE_SOURCES ${CORE_QRC_SOURCES})

add_library(Core SHARED ${CORE_SOURCES})

set_target_properties(Core PROPERTIES AUTOMOC ON)

# Native precompiled headers (CMake 3.16+)
target_precompile_headers(Core PRIVATE src/precompiled.h)

find_package(OpenGL REQUIRED)

target_link_libraries(Core
	PRIVATE
		Shared
		glm::glm
		freeimage::freeimage
		embree::embree
		${OPENGL_LIBRARIES}
		Qt5::Core
		Qt5::Widgets
		Qt5::Concurrent
		Qt5::Network
		Qt5::Xml
		Qt5::Quick
		Qt5::OpenGL
	PUBLIC
		OpenImageDenoise
)

target_include_directories(Core
	PRIVATE
		src/skcms
)

target_compile_definitions(Core
	INTERFACE CORE_EXPORT=Q_DECL_IMPORT
	PRIVATE
		CORE_EXPORT=Q_DECL_EXPORT
		SKCMS_DISABLE_HSW
		SKCMS_DISABLE_SKX
		VER_MAJOR=${APP_VER_MAJOR}
		VER_MINOR=${APP_VER_MINOR}
		VER_BUILD=${APP_VER_BUILD}
		YEAR=${VERSION_YEAR}
)

if(APPLE)
	target_compile_options(Core PRIVATE -Wno-missing-braces -Wno-reorder -Wno-switch)
	find_library(COCOA Cocoa REQUIRED)
	target_link_libraries(Core PRIVATE ${COCOA})
endif()

miray_source_group(${CORE_SOURCES})
