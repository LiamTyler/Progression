project(Converter LANGUAGES CXX C)

include(helpful_functions)
set(CODE_DIR ${PROGRESSION_DIR}/code)

include(source_files)

set(
	SRC
    ${ASSET_SRC}
    
    ${ASSET_PARSE_SRC}
    ${ECS_SRC}
	
    ${CODE_DIR}/asset/asset_cache.cpp
    ${CODE_DIR}/asset/asset_cache.hpp
    ${CODE_DIR}/asset/asset_file_database.cpp
    ${CODE_DIR}/asset/asset_file_database.hpp
    
    ${CODE_DIR}/core/bounding_box.cpp
    ${CODE_DIR}/core/bounding_box.hpp
    ${CODE_DIR}/core/camera.cpp
    ${CODE_DIR}/core/camera.hpp
    ${CODE_DIR}/core/cpu_profiling.hpp
    ${CODE_DIR}/core/engine_globals.cpp
    ${CODE_DIR}/core/engine_globals.hpp
    ${CODE_DIR}/core/feature_defines.hpp
    ${CODE_DIR}/core/frustum.cpp
    ${CODE_DIR}/core/frustum.hpp
	${CODE_DIR}/core/image_processing.cpp
    ${CODE_DIR}/core/image_processing.hpp
    ${CODE_DIR}/core/init.cpp
    ${CODE_DIR}/core/init.hpp
    ${CODE_DIR}/core/input_types.hpp
    ${CODE_DIR}/core/lights.cpp
    ${CODE_DIR}/core/lights.hpp
    ${CODE_DIR}/core/low_discrepancy_sampling.cpp
    ${CODE_DIR}/core/low_discrepancy_sampling.hpp
    ${CODE_DIR}/core/lua.cpp
    ${CODE_DIR}/core/lua.hpp
    ${CODE_DIR}/core/pixel_formats.cpp
    ${CODE_DIR}/core/pixel_formats.hpp
    ${CODE_DIR}/core/time.cpp
    ${CODE_DIR}/core/time.hpp
    ${CODE_DIR}/core/scene.cpp
    ${CODE_DIR}/core/scene.hpp
    
    ${CODE_DIR}/renderer/brdf_functions.cpp
    ${CODE_DIR}/renderer/brdf_functions.hpp
	
    ${PRIMARY_SHARED_SRC}
    ${CODE_DIR}/shared/float_conversions.hpp
    ${CODE_DIR}/shared/json_parsing.cpp
    ${CODE_DIR}/shared/json_parsing.hpp
    ${CODE_DIR}/shared/oct_encoding.hpp
    ${CODE_DIR}/shared/serializer.cpp
    ${CODE_DIR}/shared/serializer.hpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/converters/base_asset_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/converters/base_asset_converter.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/converters/font_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/converters/font_converter.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/converters/gfx_image_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/converters/gfx_image_converter.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/converters/material_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/converters/material_converter.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/converters/model_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/converters/model_converter.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/converters/pipeline_converter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/converters/pipeline_converter.hpp
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
    ${CODE_DIR}/external/msdfgen/ext/import-font.cpp
    ${CODE_DIR}/external/msdfgen/ext/import-font.h
    
    ${CODE_DIR}/external/xxHash/xxhash.c
    ${CODE_DIR}/external/xxHash/xxhash.h

    ${CODE_DIR}/external/tracy/tracy/Tracy.hpp
    ${CODE_DIR}/external/tracy/tracy/TracyVulkan.hpp
    ${CODE_DIR}/external/tracy/TracyClient.cpp
)

set(INTELLISENSE_ONLY ${IMAGELIB_EXT_FILES} ${PUGIXML_EXT_FILES} ${MSDFGEN_EXT_FILES} ${MESHOPT_EXT_FILES})

set(ALL_FILES ${SRC} ${EXTERNALS} ${INTELLISENSE_ONLY})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${ALL_FILES})
set_source_files_properties(${INTELLISENSE_ONLY} PROPERTIES HEADER_FILE_ONLY TRUE)
add_executable(${PROJECT_NAME} ${ALL_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/
	${CMAKE_CURRENT_SOURCE_DIR}/converters/
    ${COMMON_INCLUDE_DIRS} ${GLFW_INCLUDES} ${IMAGELIB_INCLUDES}
    ${LUA_INCLUDES} ${PUGIXML_INCLUDES}
    ${FREETYPE_INCLUDES} ${MSDFGEN_INCLUDES}
    ${Vulkan_INCLUDE_DIR}
    ${CODE_DIR}/external/tracy/
)

SET_TARGET_POSTFIX(${PROJECT_NAME})
SET_TARGET_COMPILE_OPTIONS_DEFAULT(${PROJECT_NAME})
target_compile_definitions(${PROJECT_NAME} PUBLIC CMAKE_DEFINE_CONVERTER ${MSDFGEN_COMPILE_DEFS} TRACY_ENABLE)
target_link_libraries(${PROJECT_NAME} PUBLIC debug
    OpenMP::OpenMP_CXX ${VULKAN_LIBS} ${IMAGELIB_LIBS_DEBUG}
    lua lz4
    ${FREETYPE_LIBS} ${MSDFGEN_LIBS} ${MESHOPT_LIBS}
)
target_link_libraries(${PROJECT_NAME} PUBLIC optimized OpenMP::OpenMP_CXX
    ${VULKAN_LIBS} ${IMAGELIB_LIBS}
    lua lz4
    ${FREETYPE_LIBS} ${MSDFGEN_LIBS} ${MESHOPT_LIBS}
)
target_link_directories(${PROJECT_NAME} PUBLIC ${CMAKE_BINARY_DIR}/lib ${CMAKE_BINARY_DIR}/bin)
