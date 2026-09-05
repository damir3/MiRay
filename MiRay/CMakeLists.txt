project(MiRay)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(SOURCES
	precompiled.h
	CameraInfo.cpp
	CameraInfo.h
	CollectionView.cpp
	CollectionView.h
	main.cpp
	MainWindow.cpp
	MainWindow.h
	MaterialInfo.cpp
	MaterialInfo.h
	NodeInfo.cpp
	NodeInfo.h
	ParamProxy.cpp
	ParamProxy.h
	RenderingProxy.cpp
	RenderingProxy.h
	RenderingParametersProxy.cpp
	RenderingParametersProxy.h
	EditNormalsProxy.cpp
	EditNormalsProxy.h
	FitToViewProxy.cpp
	FitToViewProxy.h
	AppSettingsProxy.cpp
	AppSettingsProxy.h
	PivotParametersProxy.cpp
	PivotParametersProxy.h
	UVMappingProxy.cpp
	UVMappingProxy.h
	ResourceManagerProxy.cpp
	ResourceManagerProxy.h
	RenderingLayersManagerProxy.cpp
	RenderingLayersManagerProxy.h
	SceneTree.cpp
	SceneTree.h
	SceneInfo.cpp
	SceneInfo.h
	SimpleParams.cpp
	SimpleParams.h
	SnapshotInfo.cpp
	SnapshotInfo.h
	MainWidget.cpp
	MainWidget.h
	PreviewRenderer.cpp
	PreviewRenderer.h
	ConsoleRendering.cpp
	ConsoleRendering.h
	ParametersParser.cpp
	ParametersParser.h
	BatchRenderingParametersHelper.cpp
	BatchRenderingParametersHelper.h
)

qt5_add_resources(QRC_SOURCES res/resources.qrc)
list(APPEND SOURCES ${QRC_SOURCES})

set(RESOURCES_APP "")

if(APPLE)
	set(MACOSX_BUNDLE_ICON_FILE res/icons/Application.icns)
	set(MACOSX_DOCUMENT_ICON_FILE res/icons/Document.icns)
	list(APPEND RESOURCES_APP
		${MACOSX_BUNDLE_ICON_FILE}
		${MACOSX_DOCUMENT_ICON_FILE}
	)
	set_source_files_properties(${MACOSX_BUNDLE_ICON_FILE} ${MACOSX_DOCUMENT_ICON_FILE}
		PROPERTIES MACOSX_PACKAGE_LOCATION "Resources"
	)
elseif(WIN32)
	configure_file("${CMAKE_SOURCE_DIR}/tools/win/version.rc.in" "${CMAKE_CURRENT_BINARY_DIR}/version.rc" @ONLY)
	list(APPEND SOURCES
		Platform/Win/main.rc
		"${CMAKE_CURRENT_BINARY_DIR}/version.rc"
	)
endif()

add_executable(MiRay WIN32 MACOSX_BUNDLE ${SOURCES} ${RESOURCES_APP})

# Native precompiled headers (CMake 3.16+)
target_precompile_headers(MiRay PRIVATE precompiled.h)

target_link_libraries(MiRay
	PRIVATE
		Core
		Shared
		Qt5::Core
		Qt5::Gui
		Qt5::Widgets
		Qt5::OpenGL
		Qt5::Qml
		Qt5::Quick
		Qt5::QuickWidgets
		Qt5::Concurrent
)

target_compile_definitions(MiRay PRIVATE
	VER_MAJOR=${APP_VER_MAJOR}
	VER_MINOR=${APP_VER_MINOR}
	VER_BUILD=${APP_VER_BUILD}
	VER_BETA=${APP_VER_BETA}
	YEAR=${VERSION_YEAR}
)

add_dependencies(MiRay FormatModels3D)

if(APPLE)
	find_library(CORE_FOUNDATION CoreFoundation REQUIRED)
	find_library(APP_KIT AppKit REQUIRED)
	find_library(COCOA Cocoa REQUIRED)
	target_link_libraries(MiRay PRIVATE ${CORE_FOUNDATION} ${APP_KIT} ${COCOA})

	set(INFO_PLIST_MIN_MACOS ${CMAKE_OSX_DEPLOYMENT_TARGET})
	configure_file(Platform/Mac/Info.plist.in "${CMAKE_CURRENT_BINARY_DIR}/Info.plist" @ONLY)

	set_target_properties(MiRay PROPERTIES
		MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/Info.plist"
		MACOSX_BUNDLE_GUI_IDENTIFIER "com.github.damir3.MiRay"
		MACOSX_BUNDLE_ICON_FILE "Application.icns"
		XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.github.damir3.MiRay"
		XCODE_ATTRIBUTE_CLANG_LINK_OBJC_RUNTIME "NO"
		XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME YES
	)


	# Manage macOS Bundle Frameworks & clean stale packaging artifacts (qt.conf, PlugIns)
	# leftover from tools/mac/bundle.sh so dev builds always load Qt directly from Conan cache
	set(BUNDLE_RESOURCES_DIR $<TARGET_FILE_DIR:MiRay>/../Resources)
	add_custom_command(
		TARGET MiRay POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E rm -f "${BUNDLE_RESOURCES_DIR}/qt.conf"
		COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:MiRay>/../PlugIns"
		COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:MiRay>/../Frameworks"
		COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:MiRay>/../Frameworks
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Core> $<TARGET_FILE_DIR:MiRay>/../Frameworks/
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Shared> $<TARGET_FILE_DIR:MiRay>/../Frameworks/
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:FormatModels3D> $<TARGET_FILE_DIR:MiRay>/../Frameworks/
		COMMENT "Syncing Frameworks and cleaning stale packaging artifacts for development build"
	)

	# Copy Runtime resources and QML into Bundle Resources
	add_custom_command(
		TARGET MiRay POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Resources/Preview ${BUNDLE_RESOURCES_DIR}/Preview
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Resources/Library ${BUNDLE_RESOURCES_DIR}/Library
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${OUTPUT_DIR}/qml ${BUNDLE_RESOURCES_DIR}/qml
		COMMENT "Copying Preview, Library, and QML resources into macOS Bundle Resources"
	)
elseif(WIN32)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:EMBED")

	add_custom_command(
		TARGET MiRay POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Resources/Preview ${OUTPUT_DIR}/Preview
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Resources/Library ${OUTPUT_DIR}/Library
	)
endif()

miray_source_group(${SOURCES})
