cmake_minimum_required(VERSION 3.25)
if(POLICY CMP0141)
	cmake_policy(SET CMP0141 NEW)
endif()
if(AMR_PHASE1_BUILD)
	set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded)
endif()

########################################################################################################################
## Define project
########################################################################################################################

project(
	AnimationMotionRevolution
	VERSION 1.5.4.5
	LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(AMR_PHASE1_BUILD "Use cached workspace dependencies and build regression tests" OFF)
option(AMR_MOTION_DIAGNOSTICS "Bounded motion tracing for a private diagnostic build" OFF)
if(AMR_PHASE1_BUILD)
	list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../_toolchain/vcpkg-installed/x64-windows-static-md")
	set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded)
	if(MSVC AND DEFINED ENV{USERPROFILE})
		file(TO_CMAKE_PATH "$ENV{USERPROFILE}" privacy_prefix)
		add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX>:/experimental:deterministic>"
			"$<$<COMPILE_LANGUAGE:C,CXX>:/pathmap:${privacy_prefix}=$<ANGLE-R>DoK<>")
	endif()
endif()

set(AMR_HDE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../_toolchain/minhook-v1.3.4/src/hde" CACHE PATH "Cached HDE64 instruction decoder")
if(EXISTS "${AMR_HDE_ROOT}/hde64.c")
	set(AMR_HDE_DIR "${AMR_HDE_ROOT}")
	if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../_toolchain/minhook-v1.3.4/CMakeLists.txt")
		set(FETCHCONTENT_SOURCE_DIR_HDE64 "${CMAKE_CURRENT_SOURCE_DIR}/../_toolchain/minhook-v1.3.4" CACHE PATH "Cached decoder")
		set(FETCHCONTENT_FULLY_DISCONNECTED ON CACHE BOOL "" FORCE)
	endif()
else()
	include(FetchContent)
	FetchContent_Declare(
		hde64
		GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
		GIT_TAG v1.3.4
		GIT_SHALLOW TRUE
		SOURCE_SUBDIR src/hde
	)
	FetchContent_MakeAvailable(hde64)
	set(AMR_HDE_DIR "${hde64_SOURCE_DIR}/src/hde")
endif()
add_library(amr_instruction_decoder STATIC "${AMR_HDE_DIR}/hde64.c")
target_include_directories(amr_instruction_decoder PUBLIC "${AMR_HDE_DIR}")

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

# MSVC searches quoted includes through the including-header ancestry before -I.
# Keep the historical RE/REL definitions intact, but out of the active header root.
file(GLOB_RECURSE amr_private_headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
foreach(header IN LISTS amr_private_headers)
	file(RELATIVE_PATH relative_header "${CMAKE_CURRENT_SOURCE_DIR}/include" "${header}")
	if(NOT relative_header MATCHES "^(RE|REL)/")
		configure_file("${header}" "${CMAKE_CURRENT_BINARY_DIR}/private-include/${relative_header}" COPYONLY)
	endif()
endforeach()

########################################################################################################################
## Configure target DLL
########################################################################################################################

set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)
set(ENABLE_SKYRIM_SE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_AE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_VR OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SKSE_SUPPORT_XBYAK ON CACHE BOOL "" FORCE)
set(COMMONLIB_ENABLE_IPO OFF CACHE BOOL "" FORCE)
set(COMMONLIB_PREBUILT OFF CACHE BOOL "" FORCE)
set(AMR_COMMONLIB_ROOT "$ENV{SKSE_COMMONLIB_SOURCE_ROOT}" CACHE PATH "Pinned CommonLibSSE-NG source")
if(NOT AMR_COMMONLIB_ROOT AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../_toolchain/CommonLibSSE-NG-v12.1.0/CMakeLists.txt")
	set(AMR_COMMONLIB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../_toolchain/CommonLibSSE-NG-v12.1.0")
endif()
if(AMR_COMMONLIB_ROOT AND EXISTS "${AMR_COMMONLIB_ROOT}/CMakeLists.txt")
	find_package(Git REQUIRED)
	execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD WORKING_DIRECTORY "${AMR_COMMONLIB_ROOT}"
		OUTPUT_VARIABLE amr_commonlib_commit OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
	if(NOT amr_commonlib_commit STREQUAL "8ffc2b74af015f837de53cc02b0cb470a15678e4")
		message(FATAL_ERROR "AMR requires the qualified CommonLibSSE-NG v12.1.0 revision")
	endif()
else()
	set(AMR_COMMONLIB_ROOT "${CMAKE_CURRENT_BINARY_DIR}/_deps/commonlibsse-ng-src")
	if(NOT EXISTS "${AMR_COMMONLIB_ROOT}/CMakeLists.txt")
		find_package(Git REQUIRED)
		file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_deps")
		execute_process(
			COMMAND "${GIT_EXECUTABLE}" clone --filter=blob:none --no-checkout https://github.com/alandtse/CommonLibSSE-NG.git "${AMR_COMMONLIB_ROOT}"
			COMMAND_ERROR_IS_FATAL ANY
		)
		execute_process(
			COMMAND "${GIT_EXECUTABLE}" -C "${AMR_COMMONLIB_ROOT}" checkout 8ffc2b74af015f837de53cc02b0cb470a15678e4
			COMMAND_ERROR_IS_FATAL ANY
		)
	endif()
endif()
find_package(xbyak CONFIG REQUIRED)
# Keep the established loaded-module override/name behavior, adding NG's main-module fallback.
file(READ "${AMR_COMMONLIB_ROOT}/include/REL/Module.h" amr_module_header)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ModuleDiscovery.inl" amr_module_discovery)
set(amr_module_anchor "const REX::W32::HMODULE moduleHandle = REX::W32::GetModuleHandleW(nullptr);")
string(FIND "${amr_module_header}" "${amr_module_anchor}" amr_module_position)
if(amr_module_position LESS 0)
	message(FATAL_ERROR "CommonLib module discovery anchor changed; review required")
endif()
string(REPLACE "${amr_module_anchor}" "${amr_module_discovery}" amr_module_header "${amr_module_header}")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/commonlib-overlay/REL")
file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/commonlib-overlay/REL/Module.h" CONTENT "${amr_module_header}" @ONLY)
add_subdirectory("${AMR_COMMONLIB_ROOT}" CommonLibSSE)
target_link_libraries(CommonLibSSE PUBLIC xbyak::xbyak)
target_include_directories(CommonLibSSE BEFORE PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/commonlib-overlay>")
find_package(spdlog REQUIRED CONFIG)

configure_file(cmake/PluginMetadata.cpp.in "${CMAKE_CURRENT_BINARY_DIR}/__${PROJECT_NAME}Plugin.cpp" @ONLY)
add_library(${PROJECT_NAME} SHARED ${headers} ${sources}
	"${CMAKE_CURRENT_BINARY_DIR}/__${PROJECT_NAME}Plugin.cpp" "${CMAKE_CURRENT_BINARY_DIR}/version.rc")
add_library("${PROJECT_NAME}::${PROJECT_NAME}" ALIAS "${PROJECT_NAME}")

target_include_directories(
	${PROJECT_NAME}
	BEFORE PRIVATE
		"${CMAKE_CURRENT_BINARY_DIR}/commonlib-overlay"
		"${AMR_COMMONLIB_ROOT}/include"
		"${CMAKE_CURRENT_BINARY_DIR}/private-include"
)

target_link_libraries(
	${PROJECT_NAME}
	PRIVATE
		CommonLibSSE::CommonLibSSE
		spdlog::spdlog_header_only
		amr_instruction_decoder
)
target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
if(AMR_MOTION_DIAGNOSTICS)
	target_compile_definitions(${PROJECT_NAME} PRIVATE AMR_MOTION_DIAGNOSTICS=1)
endif()
target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive- /Zc:preprocessor /EHsc /utf-8)
target_link_options(${PROJECT_NAME} PRIVATE /DEBUG:FULL /INCREMENTAL:NO /PDBALTPATH:%_PDB%)

if(AMR_PHASE1_BUILD)
	enable_testing()
	add_executable(amr_regression_tests tests/Regression.cpp "${CMAKE_CURRENT_BINARY_DIR}/version.rc")
	target_include_directories(amr_regression_tests BEFORE PRIVATE
		"${CMAKE_CURRENT_BINARY_DIR}/commonlib-overlay" "${AMR_COMMONLIB_ROOT}/include"
		"${CMAKE_CURRENT_BINARY_DIR}/private-include")
	target_compile_definitions(amr_regression_tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
	target_compile_options(amr_regression_tests PRIVATE /permissive- /Zc:preprocessor /EHsc /utf-8)
	target_link_libraries(amr_regression_tests PRIVATE CommonLibSSE::CommonLibSSE amr_instruction_decoder spdlog::spdlog_header_only)
	add_test(NAME amr_regressions COMMAND amr_regression_tests)
	add_library(amr_module_stub SHARED tests/ModuleStub.cpp "${CMAKE_CURRENT_BINARY_DIR}/version.rc")
endif()

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
		"${CMAKE_CURRENT_BINARY_DIR}/private-include/pch.h"
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
