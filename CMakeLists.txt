cmake_minimum_required(VERSION 3.21)

########################################################################################################################
## Define project
########################################################################################################################

project(
	AnimationMotionRevolution
	VERSION 1.5.4
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.rc.in
	${CMAKE_CURRENT_BINARY_DIR}/version.rc
	@ONLY
)

set(headers
	include/RE/H/hkaAnimatedReferenceFrame.h
	include/RE/H/hkaAnimation.h
	include/RE/H/hkaAnimationBinding.h
	include/RE/H/hkaAnimationControl.h
	include/RE/H/hkaAnnotationTrack.h
	include/RE/H/hkaDefaultAnimationControl.h
	include/RE/H/hkaSplineCompressedAnimation.h
	include/RE/H/hkbBehaviorGraph.h
	include/RE/H/hkbContext.h
	include/RE/H/hkbClipGenerator.h
	include/RE/H/hkbGenerator.h
	include/RE/H/hkbNode.h
	include/RE/H/hkbStateMachine.h
	include/RE/H/hkClass.h
	include/RE/H/hkQsTransform.h
	include/RE/M/MotionDataContainer.h
	include/utils/Trampoline.h
	include/utils/INISettingCollection.h
	include/utils/half.h
	include/utils/Logger.h
	include/utils/Setting.h
	include/AnimMotionHandler.h
	include/Hooks.h
	include/Motion.h
	include/pch.h
	include/Settings.h
)

set(sources
	source/utils/INISettingCollection.cpp
	source/Hooks.cpp
	source/main.cpp
	source/pch.cpp
	source/Settings.cpp
)

source_group(
	TREE
		${CMAKE_CURRENT_SOURCE_DIR}
	FILES
		${headers}
		${sources}
)

########################################################################################################################
## Configure target DLL
########################################################################################################################

set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)
set(ENABLE_SKYRIM_SE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_AE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_VR OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SKSE_SUPPORT_XBYAK ON CACHE BOOL "" FORCE)
find_package(xbyak CONFIG REQUIRED)
add_subdirectory(external/CommonLibSSE)
target_link_libraries(CommonLibSSE PUBLIC xbyak::xbyak)
include(external/CommonLibSSE/cmake/CommonLibSSE.cmake)
find_package(spdlog REQUIRED CONFIG)

add_commonlibsse_plugin(${PROJECT_NAME} AUTHOR "alexsylex" SOURCES ${headers} ${sources})
add_library("${PROJECT_NAME}::${PROJECT_NAME}" ALIAS "${PROJECT_NAME}")

target_include_directories(
	${PROJECT_NAME}
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
	${PROJECT_NAME}
	PRIVATE
		CommonLibSSE::CommonLibSSE
		spdlog::spdlog_header_only
)
target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
target_compile_options(${PROJECT_NAME} PRIVATE /permissive- /Zc:preprocessor /EHsc /utf-8)
target_link_options(${PROJECT_NAME} PRIVATE /DEBUG:FULL /INCREMENTAL:NO)

if (MSVC)
	target_link_options(
		${PROJECT_NAME}
		PRIVATE
			"$<$<CONFIG:DEBUG>:/INCREMENTAL;/OPT:NOREF;/OPT:NOICF>"
			"$<$<CONFIG:RELEASE>:/INCREMENTAL:NO;/OPT:REF;/OPT:ICF>"
	)
endif()

target_precompile_headers(
	${PROJECT_NAME}
	PRIVATE
		include/PCH.h
)

########################################################################################################################
## Optional plugin deployment
########################################################################################################################

option(AMR_ENABLE_AUTO_DEPLOY "Copy the DLL to the selected Skyrim installation after building" OFF)
set(SKYRIM_SE_DIR "" CACHE PATH "Skyrim Special Edition installation directory")

if (AMR_ENABLE_AUTO_DEPLOY)
	if (NOT EXISTS "${SKYRIM_SE_DIR}/Data/SKSE/Plugins")
		message(FATAL_ERROR "SKYRIM_SE_DIR must point to a Skyrim installation with Data/SKSE/Plugins")
	endif()

	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}>
		"${SKYRIM_SE_DIR}/Data/SKSE/Plugins/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
	)
endif()
