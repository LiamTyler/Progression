cmake_minimum_required(VERSION 3.0)
project(Converter LANGUAGES CXX C)

include(helpful_functions)
set(CODE_DIR ${PROGRESSION_DIR}/code)

set(
	SRC
    ${CODE_DIR}/asset/types/base_asset.hpp
    ${CODE_DIR}/asset/types/gfx_image.cpp
    ${CODE_DIR}/asset/types/gfx_image.hpp
    ${CODE_DIR}/asset/types/material.cpp
    ${CODE_DIR}/asset/types/material.hpp
    ${CODE_DIR}/asset/types/model.cpp
    ${CODE_DIR}/asset/types/model.hpp
    ${CODE_DIR}/asset/types/script.cpp
    ${CODE_DIR}/asset/types/script.hpp
    ${CODE_DIR}/asset/types/shader.cpp
    ${CODE_DIR}/asset/types/shader.hpp
    ${CODE_DIR}/asset/asset_manager.cpp
    ${CODE_DIR}/asset/asset_manager.hpp
    ${CODE_DIR}/asset/asset_versions.hpp
    ${CODE_DIR}/asset/image.cpp
    ${CODE_DIR}/asset/image.hpp
    ${CODE_DIR}/asset/shader_preprocessor.cpp
    ${CODE_DIR}/asset/shader_preprocessor.hpp
    
    ${CODE_DIR}//core/assert.hpp
    ${CODE_DIR}//core/bounding_box.cpp
    ${CODE_DIR}//core/bounding_box.hpp
    ${CODE_DIR}//core/camera.cpp
    ${CODE_DIR}//core/camera.hpp
    ${CODE_DIR}//core/core_defines.hpp
    ${CODE_DIR}//core/feature_defines.hpp
    ${CODE_DIR}//core/frustum.cpp
    ${CODE_DIR}//core/frustum.hpp
	${CODE_DIR}//core/image_calculate_size.cpp
    ${CODE_DIR}//core/image_calculate_size.hpp
    ${CODE_DIR}//core/input_types.hpp
    ${CODE_DIR}//core/lights.hpp
    ${CODE_DIR}//core/lua.cpp
    ${CODE_DIR}//core/lua.hpp
    ${CODE_DIR}//core/math.cpp
    ${CODE_DIR}//core/math.hpp
    ${CODE_DIR}//core/pixel_formats.cpp
    ${CODE_DIR}//core/pixel_formats.hpp
    ${CODE_DIR}//core/platform_defines.hpp
    ${CODE_DIR}//core/time.cpp
    ${CODE_DIR}//core/time.hpp
    ${CODE_DIR}//core/scene.cpp
    ${CODE_DIR}//core/scene.hpp
    
    ${PROGRESSION_DIR}/src/ecs/component_factory.cpp
    ${PROGRESSION_DIR}/src/ecs/component_factory.hpp
    ${PROGRESSION_DIR}/src/ecs/ecs.cpp
    ${PROGRESSION_DIR}/src/ecs/ecs.hpp
    ${PROGRESSION_DIR}/src/ecs/components/entity_metadata.hpp
    ${PROGRESSION_DIR}/src/ecs/components/model_renderer.hpp
    ${PROGRESSION_DIR}/src/ecs/components/transform.cpp
    ${PROGRESSION_DIR}/src/ecs/components/transform.hpp
    
    ${PROGRESSION_DIR}/src/utils/cpu_profile.hpp
    ${PROGRESSION_DIR}/src/utils/file_dependency.cpp
    ${PROGRESSION_DIR}/src/utils/file_dependency.hpp
    ${PROGRESSION_DIR}/src/utils/filesystem.cpp
    ${PROGRESSION_DIR}/src/utils/filesystem.hpp
    ${PROGRESSION_DIR}/src/utils/hash.hpp
    ${PROGRESSION_DIR}/src/utils/json_parsing.cpp
    ${PROGRESSION_DIR}/src/utils/json_parsing.hpp
    ${PROGRESSION_DIR}/src/utils/logger.cpp
    ${PROGRESSION_DIR}/src/utils/logger.hpp
    ${PROGRESSION_DIR}/src/utils/lz4_compressor.cpp
    ${PROGRESSION_DIR}/src/utils/lz4_compressor.hpp
    ${PROGRESSION_DIR}/src/utils/serializer.cpp
    ${PROGRESSION_DIR}/src/utils/serializer.hpp
    
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
    ${CMAKE_CURRENT_SOURCE_DIR}/asset_cache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/asset_cache.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/asset_file_database.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/asset_file_database.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/converters.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/converters.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/converter_main.cpp
)


set(
	EXTERNALS
    ${PROGRESSION_DIR}/ext/stb/stb_image.h
    ${PROGRESSION_DIR}/ext/stb/stb_image_write.h
    ${PROGRESSION_DIR}/ext/stb/stb_image_resize.h
    ${PROGRESSION_DIR}/ext/tinyexr/tinyexr.h
    ${PROGRESSION_DIR}/ext/tinyexr/tinyexr.cc
    ${PROGRESSION_DIR}/ext/memory_map/MemoryMapped.h
    ${PROGRESSION_DIR}/ext/memory_map/MemoryMapped.cpp
    ${PROGRESSION_DIR}/ext/getopt/getopt.c
    ${PROGRESSION_DIR}/ext/getopt/getopt.h
)

set(
	ALL_FILES
	${SRC}
	${EXTERNALS}
)

if(UNIX AND NOT APPLE)
    set(SHADERC_LIB shaderc_combined)
    set(SPIRVCROSS_LIB spirv-cross-core)
elseif(MSVC)
    set(SHADERC_LIB ${Vulkan_INCLUDE_DIR}/../Lib/shaderc_combined.lib)
    set(SPIRVCROSS_LIB ${Vulkan_INCLUDE_DIR}/../Lib/spirv-cross-core.lib)
endif()

set(LIBS lua lz4 OpenMP::OpenMP_CXX ${SHADERC_LIB} ${SPIRVCROSS_LIB} ${Vulkan_LIBRARIES})

include_directories(
    ${PROGRESSION_DIR}/converter/
    ${PROGRESSION_DIR}/src/
    ${PROGRESSION_DIR}/ext/
    ${PROGRESSION_DIR}/ext/glfw/include/
    ${PROGRESSION_DIR}/ext/lua/
	${PROGRESSION_DIR}/assets/
    ${PROGRESSION_DIR}/assets/shaders/
    ${Vulkan_INCLUDE_DIR}
)

source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${ALL_FILES})

add_executable(Converter ${ALL_FILES})
SET_TARGET_POSTFIX(Converter)
SET_TARGET_COMPILE_OPTIONS_DEFAULT(Converter)
target_link_libraries(Converter ${LIBS})
target_compile_definitions(Converter PUBLIC CMAKE_DEFINE_COMPILING_CONVERTER)
