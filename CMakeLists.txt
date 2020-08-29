cmake_minimum_required (VERSION 3.0)

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

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")

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
    #set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Werror -Wextra -Wno-implicit-fallthrough -Wshadow -Wno-unused-variable -Wno-unused-function")
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wno-unused-function")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
elseif(MSVC)
    #set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /WX")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
endif()

add_subdirectory(ext/lz4)

set(LIBS lz4)
find_package(OpenMP)
if(NOT OpenMP_CXX_FOUND)
    message("OpenMP not found! Rendering will be single threaded")
else()
    set(LIBS ${LIBS} OpenMP::OpenMP_CXX)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(NOT PROGRESSION_DIR)
    set(PROGRESSION_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()

configure_file(${PROGRESSION_DIR}/cmake/platform_defines.hpp.in ${PROGRESSION_DIR}/src/platform_defines.hpp)

set(
	SRC
	src/assert.hpp
    src/asset_manager.cpp
    src/asset_manager.hpp
    src/asset_versions.hpp
    src/base_asset.hpp
    src/core_defines.hpp
    src/gfx_image.cpp
    src/gfx_image.hpp
	src/image.cpp
    src/image.hpp
    src/material.cpp
    src/material.hpp
    src/pixel_formats.cpp
    src/pixel_formats.hpp
    src/platform_defines.hpp
    src/remote_console_server.cpp
    src/remote_console_server.hpp   
    
    src/converters/gfx_image_converter.cpp
    src/converters/gfx_image_converter.hpp
    src/converters/material_converter.cpp
    src/converters/material_converter.hpp
    
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

include_directories(
    ${CMAKE_SOURCE_DIR}/src/
    ${CMAKE_SOURCE_DIR}/ext/
)

SET(listVar "")
FOREACH(f ${ALL_FILES})
    LIST(APPEND listVar "${CMAKE_CURRENT_SOURCE_DIR}/${f}")
ENDFOREACH(f)
set(ALL_FILES ${listVar})

source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${ALL_FILES})

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_executable(Converter ${ALL_FILES})
set_target_properties(Converter PROPERTIES DEBUG_POSTFIX _d)

if(MSVC)
    target_compile_options(Converter PRIVATE "/MP")
endif()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Converter)

# link in the static libs
target_link_libraries(Converter ${LIBS})
