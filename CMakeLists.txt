cmake_minimum_required (VERSION 3.7.2) # For FindVulkan module

Project(Progression)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(helpful_functions)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Profile;Ship")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakePredefinedTargets")
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)
SET_PLATFORM_DEFINES()
find_package(Vulkan REQUIRED)
find_package(OpenMP REQUIRED)

option(PG_USE_SDL "Use SDL3 instead of GLFW (option for dev testing)" ON)

set(PROGRESSION_DIR ${CMAKE_CURRENT_SOURCE_DIR})
configure_file(${PROGRESSION_DIR}/cmake/platform_defines.hpp.in ${PROGRESSION_DIR}/code/shared/platform_defines.hpp)

SET_BIN_AND_LIB_DIRS(${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib)

set_property(DIRECTORY PROPERTY COMPILE_DEFINITIONS
    $<$<CONFIG:Debug>:CMAKE_DEFINE_DEBUG_BUILD>
    $<$<CONFIG:Release>:CMAKE_DEFINE_RELEASE_BUILD>
    $<$<CONFIG:Profile>:CMAKE_DEFINE_PROFILE_BUILD>
    $<$<CONFIG:Ship>:CMAKE_DEFINE_SHIP_BUILD>
)

set(COMMON_INCLUDE_DIRS
	${PROGRESSION_DIR}/code/
    ${PROGRESSION_DIR}/code/external/
    ${PROGRESSION_DIR}/assets/shaders/
)

add_subdirectory(code/external)
add_subdirectory(code/projects/brdf_integrate)
add_subdirectory(code/projects/engine)
add_subdirectory(code/projects/converter)
add_subdirectory(code/projects/gfximage_viewer)
add_subdirectory(code/projects/model_exporter)
add_subdirectory(code/projects/offline_renderer)
add_subdirectory(code/projects/sandbox)
if(MSVC)
    add_subdirectory(code/projects/remote_console)
endif()
