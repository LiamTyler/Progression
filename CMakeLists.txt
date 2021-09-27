cmake_minimum_required (VERSION 3.7.2) # For FindVulkan module

Project(Progression)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(LINUX_PROGRAM   "NOT_IN_USE")
set(WINDOWS_PROGRAM "NOT_IN_USE")
set(APPLE_PROGRAM   "NOT_IN_USE")

if(UNIX AND NOT APPLE)
    set(LINUX_PROGRAM   "IN_USE")
elseif(WIN32)
    set(WINDOWS_PROGRAM "IN_USE")
elseif(APPLE)
    set(APPLE_PROGRAM   "IN_USE")
endif()

find_package(Vulkan REQUIRED)
find_package(OpenMP REQUIRED)

set(PROGRESSION_DIR ${CMAKE_CURRENT_SOURCE_DIR})

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include(helpful_functions)

set(EXT_DIR ${CMAKE_SOURCE_DIR}/code/external)
#----------------------------ASSIMP--------------------------------------------
CONFIG_TIME_COMPILE(${EXT_DIR}/assimp ${CMAKE_BINARY_DIR}/ext/assimp Release)
COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/assimp/bin/Release ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib assimp)
COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/assimp/lib/Release ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib assimp)

#------------------------------LZ4---------------------------------------------
CONFIG_TIME_COMPILE(${EXT_DIR}/lz4 ${CMAKE_BINARY_DIR}/ext/lz4 Release)
COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/lz4/Release ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib lz4)

#------------------------------GLFW--------------------------------------------
CONFIG_TIME_COMPILE(${EXT_DIR}/glfw ${CMAKE_BINARY_DIR}/ext/glfw Release)
COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/glfw/src/Release ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib glfw)

#-------------------------------LUA--------------------------------------------
CONFIG_TIME_COMPILE(${EXT_DIR}/lua ${CMAKE_BINARY_DIR}/ext/lua Release)
COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/lua/Release ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib lua)

#-------------------------------IMAGELIB--------------------------------------------
CONFIG_TIME_COMPILE(${EXT_DIR}/ImageLib ${CMAKE_BINARY_DIR}/ext/ImageLib Release)
COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/ImageLib/lib ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib ImageLib)
COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/ImageLib/lib ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib tiff)


set_property(
    DIRECTORY
    PROPERTY
    COMPILE_DEFINITIONS
    $<$<CONFIG:Debug>:CMAKE_DEFINE_DEBUG_BUILD>
    $<$<CONFIG:Release>:CMAKE_DEFINE_RELEASE_BUILD>
    $<$<CONFIG:Ship>:CMAKE_DEFINE_SHIP_BUILD>
)

configure_file(${PROGRESSION_DIR}/cmake/platform_defines.hpp.in ${PROGRESSION_DIR}/code/shared/platform_defines.hpp)

add_subdirectory(code/projects/engine)
#add_subdirectory(code/projects/converter)
#add_subdirectory(code/projects/model_exporter)
