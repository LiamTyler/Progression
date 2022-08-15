cmake_minimum_required(VERSION 3.0)
project(Converter LANGUAGES CXX C)

include(helpful_functions)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
set(CODE_DIR ${PROGRESSION_DIR}/code)

include(source_files)

set(
	SRC
    ${ASSET_SRC}
    ${ASSET_PARSE_SRC}
    
    ${CODE_DIR}/asset/asset_cache.cpp
    ${CODE_DIR}/asset/asset_cache.hpp
    ${CODE_DIR}/asset/asset_file_database.cpp
    ${CODE_DIR}/asset/asset_file_database.hpp
    
    ${CODE_DIR}/core/bounding_box.cpp
    ${CODE_DIR}/core/bounding_box.hpp
    ${CODE_DIR}/core/camera.cpp
    ${CODE_DIR}/core/camera.hpp
    ${CODE_DIR}/core/feature_defines.hpp
    ${CODE_DIR}/core/frustum.cpp
    ${CODE_DIR}/core/frustum.hpp
	${CODE_DIR}/core/image_processing.cpp
    ${CODE_DIR}/core/image_processing.hpp
    ${CODE_DIR}/core/init.cpp
    ${CODE_DIR}/core/init.hpp
    ${CODE_DIR}/core/input_types.hpp
    ${CODE_DIR}/core/lights.hpp
    ${CODE_DIR}/core/lua.cpp
    ${CODE_DIR}/core/lua.hpp
    ${CODE_DIR}/core/pixel_formats.cpp
    ${CODE_DIR}/core/pixel_formats.hpp
    ${CODE_DIR}/core/time.cpp
    ${CODE_DIR}/core/time.hpp
    ${CODE_DIR}/core/scene.cpp
    ${CODE_DIR}/core/scene.hpp
    
    ${ECS_SRC}
    
    ${PRIMARY_SHARED_SRC}
    ${CODE_DIR}/shared/float_conversions.hpp
    ${CODE_DIR}/shared/json_parsing.cpp
    ${CODE_DIR}/shared/json_parsing.hpp
    ${CODE_DIR}/shared/serializer.cpp
    ${CODE_DIR}/shared/serializer.hpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/converters/base_asset_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/converters/base_asset_converter.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/converters/gfx_image_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/converters/gfx_image_converter.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/converters/material_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/converters/material_converter.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/converters/model_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/converters/model_converter.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/converters/script_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/converters/script_converter.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/converters/shader_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/converters/shader_converter.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/converters.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/converters.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/converter_main.cpp
)

set(
	EXTERNALS
    ${CODE_DIR}/external/memory_map/MemoryMapped.h
    ${CODE_DIR}/external/memory_map/MemoryMapped.cpp
    ${CODE_DIR}/external/getopt/getopt.c
    ${CODE_DIR}/external/getopt/getopt.h
)

set(ALL_FILES ${SRC} ${EXTERNALS} ${IMAGELIB_EXT_FILES})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${ALL_FILES})
set_source_files_properties(${IMAGELIB_EXT_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
add_executable(Converter ${ALL_FILES})

target_include_directories(Converter PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/
	${CMAKE_CURRENT_SOURCE_DIR}/converters/
    ${COMMON_INCLUDE_DIRS} ${GLFW_INCLUDES} ${IMAGELIB_INCLUDES} ${LUA_INCLUDES} ${Vulkan_INCLUDE_DIR}
)

SET_TARGET_POSTFIX(Converter)
SET_TARGET_COMPILE_OPTIONS_DEFAULT(Converter)
target_compile_definitions(Converter PUBLIC CMAKE_DEFINE_CONVERTER)
target_link_libraries(Converter PUBLIC debug OpenMP::OpenMP_CXX ${VULKAN_LIBS} ${IMAGELIB_LIBS_DEBUG} lua lz4)
target_link_libraries(Converter PUBLIC optimized OpenMP::OpenMP_CXX ${VULKAN_LIBS} ${IMAGELIB_LIBS} lua lz4)
target_link_directories(${PROJECT_NAME} PUBLIC ${CMAKE_BINARY_DIR}/lib)