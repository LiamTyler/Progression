cmake_minimum_required (VERSION 3.7.2) # For FindVulkan module

project(Converter LANGUAGES CXX C)

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

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)
set(CAMKE_COMPILE_PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)

if(UNIX AND NOT APPLE)
    #set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Werror -Wextra -Wno-implicit-fallthrough -Wshadow -Wno-unused-variable -Wno-unused-function")
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wno-unused-function")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
elseif(MSVC)
    #set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /WX")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
endif()


set(ASSIMP_BUILD_TESTS        OFF CACHE BOOL " " FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL " " FORCE)
# disable a bunch of importers that we wont use. Just add
# them to the list if you want it enabled again
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL " " FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER     ON CACHE BOOL "Enable format OBJ"     FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER     ON CACHE BOOL "Enable format FBX"     FORCE)
set(ASSIMP_BUILD_3DS_IMPORTER     ON CACHE BOOL "Enable format 3DS"     FORCE)
set(ASSIMP_BUILD_PLY_IMPORTER     ON CACHE BOOL "Enable format PLY"     FORCE)
set(ASSIMP_BUILD_COLLADA_IMPORTER ON CACHE BOOL "Enable format COLLADA" FORCE)
set(ASSIMP_BUILD_STL_IMPORTER     ON CACHE BOOL "Enable format STL"     FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER    ON CACHE BOOL "Enable format GLTF"    FORCE)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL " " FORCE)
# assimp doesnt compile if all exporters are disabled -.-
set(ASSIMP_BUILD_OBJ_EXPORTER    ON CACHE BOOL "Enable format OBJ"    FORCE)
add_subdirectory(ext/assimp)

add_subdirectory(ext/lz4)

set(SHADERC_SKIP_INSTALL OFF CACHE BOOL " " FORCE)
set(SHADERC_SKIP_TESTS   ON  CACHE BOOL " " FORCE)
#add_subdirectory(ext/shaderc)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(NOT PROGRESSION_DIR)
    set(PROGRESSION_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()

add_subdirectory(tools)

configure_file(${PROGRESSION_DIR}/cmake/platform_defines.hpp.in ${PROGRESSION_DIR}/src/platform_defines.hpp)

set(
	SRC
	src/assert.hpp
    src/assetTypes/base_asset.hpp
    src/assetTypes/gfx_image.cpp
    src/assetTypes/gfx_image.hpp
    src/assetTypes/material.cpp
    src/assetTypes/material.hpp
    src/assetTypes/model.cpp
    src/assetTypes/model.hpp
    src/assetTypes/script.cpp
    src/assetTypes/script.hpp
    src/assetTypes/shader.cpp
    src/assetTypes/shader.hpp
    src/asset_manager.cpp
    src/asset_manager.hpp
    src/asset_versions.hpp
    
    src/core_defines.hpp
    
	src/image.cpp
    src/image.hpp
    src/pixel_formats.cpp
    src/pixel_formats.hpp
    src/platform_defines.hpp
    src/remote_console_server.cpp
    src/remote_console_server.hpp   
    
    src/converters/gfx_image_converter.cpp
    src/converters/gfx_image_converter.hpp
    src/converters/material_converter.cpp
    src/converters/material_converter.hpp
    src/converters/model_converter.cpp
    src/converters/model_converter.hpp
    src/converters/script_converter.cpp
    src/converters/script_converter.hpp
    src/converters/shader_converter.cpp
    src/converters/shader_converter.hpp
    
    src/utils/cpu_profile.hpp
    src/utils/file_dependency.cpp
    src/utils/file_dependency.hpp
    src/utils/filesystem.cpp
    src/utils/filesystem.hpp
    src/utils/json_parsing.cpp
    src/utils/json_parsing.hpp
    src/utils/logger.cpp
    src/utils/logger.hpp
    src/utils/lz4_compressor.cpp
    src/utils/lz4_compressor.hpp
    src/utils/serializer.cpp
    src/utils/serializer.hpp
    
    src/main.cpp
)


set(
	EXTERNALS
    ext/stb/stb_image.h
    ext/stb/stb_image_write.h
    ext/stb/stb_image_resize.h
    ext/tinyexr/tinyexr.h
    ext/tinyexr/tinyexr.cc
    ext/memory_map/MemoryMapped.h
    ext/memory_map/MemoryMapped.cpp
)

set(
	ALL_FILES
	${SRC}
	${EXTERNALS}
)

set(LIBS lz4)
find_package(OpenMP)
if(NOT OpenMP_CXX_FOUND)
    message("OpenMP not found! Rendering will be single threaded")
else()
    list(APPEND LIBS OpenMP::OpenMP_CXX)
endif()

set(SHADERC_DIR ${Vulkan_INCLUDE_DIR}/..)
#set(SHADERC_LIB ${SHADERC_DIR}/Lib/shaderc_combined.lib ${SHADERC_DIR}/Lib/shaderc_util.lib ${SHADERC_DIR}/Lib/glslang.lib)
set(SHADERC_LIB ${SHADERC_DIR}/Lib/shaderc_combined.lib)
message(${SHADERC_DIR})
message(${SHADERC_LIB})
list(APPEND LIBS ${SHADERC_LIB})

include_directories(
    ${CMAKE_SOURCE_DIR}/src/
    ${CMAKE_SOURCE_DIR}/ext/
    ${CMAKE_SOURCE_DIR}/ext/shaderc/
    ${Vulkan_INCLUDE_DIR}
    #${CMAKE_SOURCE_DIR}/ext/shaderc/
    #${CMAKE_SOURCE_DIR}/ext/shaderc/libshaderc/include/
)

SET(listVar "")
FOREACH(f ${ALL_FILES})
    LIST(APPEND listVar "${CMAKE_CURRENT_SOURCE_DIR}/${f}")
ENDFOREACH(f)
set(ALL_FILES ${listVar})

source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${ALL_FILES})

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_ITERATOR_DEBUG_LEVEL=0)
endif()

add_executable(Converter ${ALL_FILES})
set_target_properties(Converter PROPERTIES DEBUG_POSTFIX _d)
add_dependencies(Converter lz4)

if(MSVC)
    target_compile_options(Converter PRIVATE "/MD")
    target_compile_options(Converter PRIVATE "/MP")
endif()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Converter)

# link in the static libs
target_link_libraries(Converter ${LIBS})
