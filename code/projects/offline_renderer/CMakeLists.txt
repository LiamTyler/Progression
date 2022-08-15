cmake_minimum_required(VERSION 3.0)
project(OfflineRenderer LANGUAGES CXX C)

include(helpful_functions)
#set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
set(CODE_DIR ${PROGRESSION_DIR}/code)
include(source_files)

set(
	SRC
    ${ASSET_SRC}
    ${ASSET_PARSE_SRC}
    
    ${CODE_DIR}/asset/asset_file_database.cpp
    ${CODE_DIR}/asset/asset_file_database.hpp    
    ${CODE_DIR}/asset/shader_preprocessor.cpp
    ${CODE_DIR}/asset/shader_preprocessor.hpp
    
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
    ${CODE_DIR}/shared/color_spaces.hpp
    ${CODE_DIR}/shared/float_conversions.hpp
    ${CODE_DIR}/shared/json_parsing.cpp
    ${CODE_DIR}/shared/json_parsing.hpp
    ${CODE_DIR}/shared/serializer.cpp
    ${CODE_DIR}/shared/serializer.hpp
	
	${CMAKE_CURRENT_SOURCE_DIR}/asset/pt_material.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/asset/pt_material.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/asset/pt_model.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/asset/pt_model.hpp
    #${CMAKE_CURRENT_SOURCE_DIR}/resource/resource.hpp
    #${CMAKE_CURRENT_SOURCE_DIR}/resource/resource_manager.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/resource/resource_manager.hpp
    #${CMAKE_CURRENT_SOURCE_DIR}/resource/skybox.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/resource/skybox.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/asset/pt_image.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/asset/pt_image.hpp
	
    ${CMAKE_CURRENT_SOURCE_DIR}/anti_aliasing.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/anti_aliasing.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bvh.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bvh.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/intersection_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/intersection_tests.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pt_lights.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/pt_lights.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/pt_scene.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/pt_scene.hpp
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
target_compile_definitions(OfflineRenderer PUBLIC CMAKE_DEFINE_OFFLINE_RENDERER)
target_link_libraries(OfflineRenderer PUBLIC OpenMP::OpenMP_CXX ${VULKAN_LIBS} ${IMAGELIB_LIBS} lua lz4)
target_link_directories(${PROJECT_NAME} PUBLIC ${CMAKE_BINARY_DIR}/lib)