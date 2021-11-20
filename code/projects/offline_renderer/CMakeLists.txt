cmake_minimum_required(VERSION 3.0)
project(OfflineRenderer LANGUAGES CXX C)

include(helpful_functions)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
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
    ${CODE_DIR}/asset/asset_manager.cpp
    ${CODE_DIR}/asset/asset_manager.hpp
    ${CODE_DIR}/asset/asset_versions.hpp
    
    ${CODE_DIR}/core/bounding_box.cpp
    ${CODE_DIR}/core/bounding_box.hpp
    ${CODE_DIR}/core/camera.cpp
    ${CODE_DIR}/core/camera.hpp
    ${CODE_DIR}/core/feature_defines.hpp
    ${CODE_DIR}/core/frustum.cpp
    ${CODE_DIR}/core/frustum.hpp
    ${CODE_DIR}/core/lights.hpp
    ${CODE_DIR}/core/pixel_formats.cpp
    ${CODE_DIR}/core/pixel_formats.hpp
    ${CODE_DIR}/core/time.cpp
    ${CODE_DIR}/core/time.hpp
    ${CODE_DIR}/core/scene.cpp
    ${CODE_DIR}/core/scene.hpp
    
    ${CODE_DIR}/ecs/component_factory.cpp
    ${CODE_DIR}/ecs/component_factory.hpp
    ${CODE_DIR}/ecs/ecs.cpp
    ${CODE_DIR}/ecs/ecs.hpp
    ${CODE_DIR}/ecs/components/entity_metadata.hpp
    ${CODE_DIR}/ecs/components/model_renderer.hpp
    ${CODE_DIR}/ecs/components/transform.cpp
    ${CODE_DIR}/ecs/components/transform.hpp
    
    ${CODE_DIR}/shared/bitops.hpp
    ${CODE_DIR}/shared/core_defines.hpp
    ${CODE_DIR}/shared/file_dependency.cpp
    ${CODE_DIR}/shared/file_dependency.hpp
    ${CODE_DIR}/shared/filesystem.cpp
    ${CODE_DIR}/shared/filesystem.hpp
    ${CODE_DIR}/shared/float_conversions.hpp
    ${CODE_DIR}/shared/json_parsing.cpp
    ${CODE_DIR}/shared/json_parsing.hpp
    ${CODE_DIR}/shared/logger.cpp
    ${CODE_DIR}/shared/logger.hpp
    ${CODE_DIR}/shared/math.hpp
    ${CODE_DIR}/shared/noncopyable.hpp
    ${CODE_DIR}/shared/random.cpp
    ${CODE_DIR}/shared/random.hpp
    ${CODE_DIR}/shared/serializer.cpp
    ${CODE_DIR}/shared/serializer.hpp
	
	${CMAKE_CURRENT_SOURCE_DIR}/resource/material.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/material.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/model.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/model.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/resource.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/resource_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/resource_manager.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/skybox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/skybox.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/texture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resource/texture.hpp
	
    ${CMAKE_CURRENT_SOURCE_DIR}/anti_aliasing.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/anti_aliasing.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bvh.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bvh.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/intersection_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/intersection_tests.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/offline_scene.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/offline_scene.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/path_tracer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/path_tracer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sampling.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sampling.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/shapes.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/shapes.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tonemap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tonemap.hpp
	
	
    ${CMAKE_CURRENT_SOURCE_DIR}/offline_renderer_main.cpp
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
add_executable(OfflineRenderer ${ALL_FILES})

target_include_directories(OfflineRenderer PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/
    ${COMMON_INCLUDE_DIRS} ${IMAGELIB_INCLUDES} ${Vulkan_INCLUDE_DIR} ${LUA_INCLUDES} #${GLFW_INCLUDES} 
)

SET_TARGET_POSTFIX(OfflineRenderer)
SET_TARGET_COMPILE_OPTIONS_DEFAULT(OfflineRenderer)
target_compile_definitions(OfflineRenderer PUBLIC CMAKE_DEFINE_COMPILING_CONVERTER)
#target_link_libraries(OfflineRenderer PUBLIC OpenMP::OpenMP_CXX ${VULKAN_LIBS} ${IMAGELIB_LIBS} lua lz4)
target_link_libraries(OfflineRenderer PUBLIC OpenMP::OpenMP_CXX ${IMAGELIB_LIBS} lz4 lua)
target_link_directories(${PROJECT_NAME} PUBLIC ${CMAKE_BINARY_DIR}/lib)