project(Converter LANGUAGES CXX C)

include(set_postfix)
include(set_compile_options_default)

set(
	SRC
    ${PROGRESSION_DIR}/src/asset/types/base_asset.hpp
    ${PROGRESSION_DIR}/src/asset/types/gfx_image.cpp
    ${PROGRESSION_DIR}/src/asset/types/gfx_image.hpp
    ${PROGRESSION_DIR}/src/asset/types/material.cpp
    ${PROGRESSION_DIR}/src/asset/types/material.hpp
    ${PROGRESSION_DIR}/src/asset/types/model.cpp
    ${PROGRESSION_DIR}/src/asset/types/model.hpp
    ${PROGRESSION_DIR}/src/asset/types/script.cpp
    ${PROGRESSION_DIR}/src/asset/types/script.hpp
    ${PROGRESSION_DIR}/src/asset/types/shader.cpp
    ${PROGRESSION_DIR}/src/asset/types/shader.hpp
    ${PROGRESSION_DIR}/src/asset/asset_manager.cpp
    ${PROGRESSION_DIR}/src/asset/asset_manager.hpp
    ${PROGRESSION_DIR}/src/asset/asset_versions.hpp
    ${PROGRESSION_DIR}/src/asset/image.cpp
    ${PROGRESSION_DIR}/src/asset/image.hpp
    ${PROGRESSION_DIR}/src/asset/shader_preprocessor.cpp
    ${PROGRESSION_DIR}/src/asset/shader_preprocessor.hpp
    
    ${PROGRESSION_DIR}/src/core/assert.hpp
    ${PROGRESSION_DIR}/src/core/core_defines.hpp
    ${PROGRESSION_DIR}/src/core/image_calculate_size.cpp
    ${PROGRESSION_DIR}/src/core/image_calculate_size.hpp
    ${PROGRESSION_DIR}/src/core/pixel_formats.cpp
    ${PROGRESSION_DIR}/src/core/pixel_formats.hpp
    ${PROGRESSION_DIR}/src/core/platform_defines.hpp
    ${PROGRESSION_DIR}/src/core/time.cpp
    ${PROGRESSION_DIR}/src/core/time.hpp
    
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
    
    ${CMAKE_CURRENT_SOURCE_DIR}/converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/converter.hpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
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
	${PROGRESSION_DIR}/assets/
    ${PROGRESSION_DIR}/assets/shaders/
    ${PROGRESSION_DIR}/ext/lua/
    ${Vulkan_INCLUDE_DIR}
)

source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${ALL_FILES})

add_executable(Converter ${ALL_FILES})
SET_TARGET_POSTFIX(Converter)
SET_TARGET_COMPILE_OPTIONS_DEFAULT(Converter)
target_link_libraries(Converter ${LIBS})
target_compile_definitions(Converter PUBLIC CMAKE_DEFINE_COMPILING_CONVERTER)
