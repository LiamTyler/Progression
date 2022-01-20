project(Engine LANGUAGES CXX C)

include(helpful_functions)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Ship")
SET_BIN_AND_LIB_DIRS(${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_SHIP ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_SHIP ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_SHIP ${CMAKE_BINARY_DIR}/lib)

if(UNIX AND NOT APPLE)
elseif(MSVC)
    set(CMAKE_C_FLAGS_SHIP "" CACHE STRING "" FORCE)
    set(CMAKE_EXE_LINKER_FLAGS_SHIP "" CACHE STRING "" FORCE)
    set(CMAKE_SHARED_LINKER_FLAGS_SHIP "" CACHE STRING "" FORCE)
    mark_as_advanced(CMAKE_CXX_FLAGS_SHIP CMAKE_C_FLAGS_SHIP CMAKE_EXE_LINKER_FLAGS_SHIP CMAKE_SHARED_LINKER_FLAGS_SHIP)
endif()

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
    ${CODE_DIR}/asset/shader_preprocessor.cpp
    ${CODE_DIR}/asset/shader_preprocessor.hpp
    
    ${CODE_DIR}/core/bounding_box.cpp
    ${CODE_DIR}/core/bounding_box.hpp
    ${CODE_DIR}/core/camera.cpp
    ${CODE_DIR}/core/camera.hpp
    ${CODE_DIR}/core/feature_defines.hpp
    ${CODE_DIR}/core/frustum.cpp
    ${CODE_DIR}/core/frustum.hpp
    ${CODE_DIR}/core/init.cpp
    ${CODE_DIR}/core/init.hpp
    ${CODE_DIR}/core/input.cpp
    ${CODE_DIR}/core/input.hpp
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
    ${CODE_DIR}/core/window.cpp
    ${CODE_DIR}/core/window.hpp
    
    ${CODE_DIR}/ecs/component_factory.cpp
    ${CODE_DIR}/ecs/component_factory.hpp
    ${CODE_DIR}/ecs/ecs.cpp
    ${CODE_DIR}/ecs/ecs.hpp
    ${CODE_DIR}/ecs/components/entity_metadata.hpp
    ${CODE_DIR}/ecs/components/model_renderer.hpp
    ${CODE_DIR}/ecs/components/transform.cpp
    ${CODE_DIR}/ecs/components/transform.hpp
    
    ${CODE_DIR}/renderer/graphics_api/buffer.cpp
    ${CODE_DIR}/renderer/graphics_api/buffer.hpp
    ${CODE_DIR}/renderer/graphics_api/command_buffer.cpp
    ${CODE_DIR}/renderer/graphics_api/command_buffer.hpp
    ${CODE_DIR}/renderer/graphics_api/descriptor.cpp
    ${CODE_DIR}/renderer/graphics_api/descriptor.hpp
    ${CODE_DIR}/renderer/graphics_api/device.cpp
    ${CODE_DIR}/renderer/graphics_api/device.hpp
    ${CODE_DIR}/renderer/graphics_api/framebuffer.cpp
    ${CODE_DIR}/renderer/graphics_api/framebuffer.hpp
    ${CODE_DIR}/renderer/graphics_api/gpu_profiling.cpp
    ${CODE_DIR}/renderer/graphics_api/gpu_profiling.hpp
    ${CODE_DIR}/renderer/graphics_api/limits.hpp
    ${CODE_DIR}/renderer/graphics_api/pg_to_vulkan_types.hpp
    ${CODE_DIR}/renderer/graphics_api/physical_device.cpp
    ${CODE_DIR}/renderer/graphics_api/physical_device.hpp
    ${CODE_DIR}/renderer/graphics_api/pipeline.cpp
    ${CODE_DIR}/renderer/graphics_api/pipeline.hpp
    ${CODE_DIR}/renderer/graphics_api/render_pass.cpp
    ${CODE_DIR}/renderer/graphics_api/render_pass.hpp
    ${CODE_DIR}/renderer/graphics_api/sampler.cpp
    ${CODE_DIR}/renderer/graphics_api/sampler.hpp
    ${CODE_DIR}/renderer/graphics_api/swapchain.cpp
    ${CODE_DIR}/renderer/graphics_api/swapchain.hpp
    ${CODE_DIR}/renderer/graphics_api/synchronization.cpp
    ${CODE_DIR}/renderer/graphics_api/synchronization.hpp
    ${CODE_DIR}/renderer/graphics_api/texture.cpp
    ${CODE_DIR}/renderer/graphics_api/texture.hpp
    ${CODE_DIR}/renderer/graphics_api/vertex_descriptor.cpp
    ${CODE_DIR}/renderer/graphics_api/vertex_descriptor.hpp
    
    ${CODE_DIR}/renderer/debug_marker.cpp
    ${CODE_DIR}/renderer/debug_marker.hpp
    ${CODE_DIR}/renderer/graphics_api.hpp
    ${CODE_DIR}/renderer/render_system.cpp
    ${CODE_DIR}/renderer/render_system.hpp
    ${CODE_DIR}/renderer/r_globals.cpp
    ${CODE_DIR}/renderer/r_globals.hpp
    ${CODE_DIR}/renderer/r_init.cpp
    ${CODE_DIR}/renderer/r_init.hpp
	${CODE_DIR}/renderer/r_texture_manager.cpp
    ${CODE_DIR}/renderer/r_texture_manager.hpp
    ${CODE_DIR}/renderer/vulkan.cpp
    ${CODE_DIR}/renderer/vulkan.hpp
	${CODE_DIR}/renderer/rendergraph/r_rendergraph.cpp
    ${CODE_DIR}/renderer/rendergraph/r_rendergraph.hpp
	
	${PROGRESSION_DIR}/assets/shaders/c_shared/structs.h
	${PROGRESSION_DIR}/assets/shaders/c_shared/defines.h
    
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

    ${CMAKE_CURRENT_SOURCE_DIR}/engine_main.cpp
)

set(
	EXTERNALS
    ${CODE_DIR}/external/memory_map/MemoryMapped.h
    ${CODE_DIR}/external/memory_map/MemoryMapped.cpp
)

set(ALL_FILES ${SRC} ${EXTERNALS} ${IMAGELIB_EXT_FILES})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${ALL_FILES})
set_source_files_properties(${IMAGELIB_EXT_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
add_executable(${PROJECT_NAME} ${ALL_FILES})
SET_TARGET_COMPILE_OPTIONS_DEFAULT(${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES
	DEBUG_POSTFIX _d
	RELEASE_POSTFIX _r
	SHIP_POSTFIX _s
)
target_include_directories(${PROJECT_NAME} PUBLIC
    ${COMMON_INCLUDE_DIRS} ${GLFW_INCLUDES} ${IMAGELIB_INCLUDES} ${LUA_INCLUDES} ${Vulkan_INCLUDE_DIR}
    ${PROGRESSION_DIR}/assets/
    ${PROGRESSION_DIR}/assets/shaders/
)
target_link_libraries(${PROJECT_NAME} PUBLIC OpenMP::OpenMP_CXX ${VULKAN_LIBS} ${IMAGELIB_LIBS} ${GLFW_LIBS} lua lz4)
target_link_directories(${PROJECT_NAME} PUBLIC ${CMAKE_BINARY_DIR}/lib)
