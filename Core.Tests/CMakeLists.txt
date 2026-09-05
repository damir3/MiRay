project(Core.Tests)

set(CORE_TESTS_SOURCES
	Helpers.h
	precompiled.h
	Helpers.cpp
	Tests.cpp
	Test_Camera.cpp
	Test_IOR.cpp
	Test_Materials.cpp
	Test_RenderLayers.cpp
	Test_Scene.cpp
	Test_Snapshots.cpp
	Test_Utils.cpp
	Test_UpdateFileNames.cpp
	Test_Parameters.cpp
	Test_ColorUtils.cpp
	Test_Images.cpp

	../Core/src/Materials/IORReader.cpp
	../Core/src/Utils.cpp
)

qt5_add_resources(CORE_TESTS_QRC_SOURCES res/resources.qrc)
list(APPEND CORE_TESTS_SOURCES ${CORE_TESTS_QRC_SOURCES})

add_executable(Core.Tests ${CORE_TESTS_SOURCES})

set_target_properties(Core.Tests PROPERTIES AUTOMOC ON)

# Native precompiled headers (CMake 3.16+)
target_precompile_headers(Core.Tests PRIVATE precompiled.h)

target_link_libraries(Core.Tests
	PRIVATE
		Core
		Shared
		gtest::gtest
		glm::glm
		Qt5::Core
		Qt5::Network
		Qt5::Widgets
		Qt5::Xml
)

target_compile_definitions(Core.Tests PRIVATE CORE_EXPORT=Q_DECL_IMPORT)

if(APPLE)
	target_compile_options(Core.Tests PRIVATE -Wno-reorder)
endif()

miray_source_group(${CORE_TESTS_SOURCES})

add_test(NAME Core.Tests COMMAND Core.Tests)
