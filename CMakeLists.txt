cmake_minimum_required (VERSION 3.7.2) # For FindVulkan module

Project(Progression)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Ship")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_SHIP ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_SHIP ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_SHIP ${CMAKE_BINARY_DIR}/lib)

if(UNIX AND NOT APPLE)
elseif(MSVC)
    set(CMAKE_C_FLAGS_SHIP "" CACHE STRING "" FORCE)
    set(CMAKE_EXE_LINKER_FLAGS_SHIP "" CACHE STRING "" FORCE)
    set(CMAKE_SHARED_LINKER_FLAGS_SHIP "" CACHE STRING "" FORCE)
    mark_as_advanced(CMAKE_CXX_FLAGS_SHIP CMAKE_C_FLAGS_SHIP CMAKE_EXE_LINKER_FLAGS_SHIP CMAKE_SHARED_LINKER_FLAGS_SHIP)
    #add_link_options("/NODEFAULTLIB:library")
endif()


#----------------------------ASSIMP--------------------------------------------
set(ASSIMP_BUILD_TESTS        OFF CACHE BOOL " " FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL " " FORCE)
# disable a bunch of importers that we wont use. Just add them to the list if you want it enabled again
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL " " FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER     ON CACHE BOOL "Enable format OBJ"     FORCE)
# turn off most importers during CI to speed up builds
if(NOT DEFINED ENV{PG_IS_CI_BUILD})
set(ASSIMP_BUILD_FBX_IMPORTER     ON CACHE BOOL "Enable format FBX"     FORCE)
set(ASSIMP_BUILD_3DS_IMPORTER     ON CACHE BOOL "Enable format 3DS"     FORCE)
set(ASSIMP_BUILD_PLY_IMPORTER     ON CACHE BOOL "Enable format PLY"     FORCE)
set(ASSIMP_BUILD_COLLADA_IMPORTER ON CACHE BOOL "Enable format COLLADA" FORCE)
set(ASSIMP_BUILD_STL_IMPORTER     ON CACHE BOOL "Enable format STL"     FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER    ON CACHE BOOL "Enable format GLTF"    FORCE)
endif()
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL " " FORCE)
# assimp doesnt compile if all exporters are disabled -.-
set(ASSIMP_BUILD_OBJ_EXPORTER    ON CACHE BOOL "Enable format OBJ"    FORCE)
add_subdirectory(ext/assimp)
#------------------------------------------------------------------------------


#------------------------------LZ4---------------------------------------------
add_subdirectory(ext/lz4)
#------------------------------------------------------------------------------

#------------------------------GLFW--------------------------------------------
set(GLFW_BUILD_EXAMPLES  OFF CACHE BOOL " " FORCE)
set(GLFW_BUILD_TESTS     OFF CACHE BOOL " " FORCE)
set(GLFW_BUILD_DOCS      OFF CACHE BOOL " " FORCE)
add_subdirectory(ext/glfw)
#------------------------------------------------------------------------------

#-------------------------------LUA--------------------------------------------
if(UNIX AND NOT APPLE)
    add_definitions(-DLUA_USE_POSIX)
endif()
file(GLOB_RECURSE LUA_SRC ext/lua/*.c)
list(REMOVE_ITEM LUA_SRC "${CMAKE_SOURCE_DIR}/ext/lua/lua.c")
list(REMOVE_ITEM LUA_SRC "${CMAKE_SOURCE_DIR}/ext/lua/luac.c")
add_library(lua STATIC ${LUA_SRC})
#------------------------------------------------------------------------------


list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(NOT PROGRESSION_DIR)
    set(PROGRESSION_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()

set_property(
    DIRECTORY
    PROPERTY
    COMPILE_DEFINITIONS
    $<$<CONFIG:Debug>:CMAKE_DEFINE_DEBUG_BUILD>
    $<$<CONFIG:Release>:CMAKE_DEFINE_RELEASE_BUILD>
    $<$<CONFIG:Ship>:CMAKE_DEFINE_SHIP_BUILD>
)

configure_file(${PROGRESSION_DIR}/cmake/platform_defines.hpp.in ${PROGRESSION_DIR}/src/core/platform_defines.hpp)

add_subdirectory(engine)
add_subdirectory(converter)
add_subdirectory(tools)
