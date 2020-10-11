project(Engine LANGUAGES CXX C)

include(set_postfix)
include(set_compile_options_default)

set(
	SRC
    
    ${PROGRESSION_DIR}/src/engine_init.cpp
    ${PROGRESSION_DIR}/src/engine_init.hpp
    
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
    
    ${PROGRESSION_DIR}/src/core/assert.hpp
    ${PROGRESSION_DIR}/src/core/core_defines.hpp
    ${PROGRESSION_DIR}/src/core/feature_defines.hpp
    ${PROGRESSION_DIR}/src/core/input.cpp
    ${PROGRESSION_DIR}/src/core/input.hpp
    ${PROGRESSION_DIR}/src/core/input_types.hpp
    ${PROGRESSION_DIR}/src/core/pixel_formats.cpp
    ${PROGRESSION_DIR}/src/core/pixel_formats.hpp
    ${PROGRESSION_DIR}/src/core/platform_defines.hpp
    ${PROGRESSION_DIR}/src/core/time.cpp
    ${PROGRESSION_DIR}/src/core/time.hpp
    ${PROGRESSION_DIR}/src/core/window.cpp
    ${PROGRESSION_DIR}/src/core/window.hpp
    
    ${PROGRESSION_DIR}/src/renderer/graphics_api/buffer.cpp
    ${PROGRESSION_DIR}/src/renderer/graphics_api/buffer.hpp
    ${PROGRESSION_DIR}/src/renderer/graphics_api/command_buffer.cpp
    ${PROGRESSION_DIR}/src/renderer/graphics_api/command_buffer.hpp
    ${PROGRESSION_DIR}/src/renderer/graphics_api/descriptor.cpp
    ${PROGRESSION_DIR}/src/renderer/graphics_api/descriptor.hpp
    ${PROGRESSION_DIR}/src/renderer/graphics_api/device.cpp
    ${PROGRESSION_DIR}/src/renderer/graphics_api/device.hpp
    ${PROGRESSION_DIR}/src/renderer/graphics_api/framebuffer.cpp
    ${PROGRESSION_DIR}/src/renderer/graphics_api/framebuffer.hpp
    ${PROGRESSION_DIR}/src/renderer/graphics_api/gpu_profiling.cpp
    ${PROGRESSION_DIR}/src/renderer/graphics_api/gpu_profiling.hpp
    ${PROGRESSION_DIR}/src/renderer/graphics_api/limits.hpp
    ${PROGRESSION_DIR}/src/renderer/graphics_api/physical_device.cpp
    ${PROGRESSION_DIR}/src/renderer/graphics_api/physical_device.hpp
    ${PROGRESSION_DIR}/src/renderer/graphics_api/pipeline.cpp
    ${PROGRESSION_DIR}/src/renderer/graphics_api/pipeline.hpp
    ${PROGRESSION_DIR}/src/renderer/graphics_api/render_pass.cpp
    ${PROGRESSION_DIR}/src/renderer/graphics_api/render_pass.hpp
    ${PROGRESSION_DIR}/src/renderer/graphics_api/sampler.cpp
    ${PROGRESSION_DIR}/src/renderer/graphics_api/sampler.hpp
    ${PROGRESSION_DIR}/src/renderer/graphics_api/swapchain.cpp
    ${PROGRESSION_DIR}/src/renderer/graphics_api/swapchain.hpp
    ${PROGRESSION_DIR}/src/renderer/graphics_api/synchronization.cpp
    ${PROGRESSION_DIR}/src/renderer/graphics_api/synchronization.hpp
    ${PROGRESSION_DIR}/src/renderer/graphics_api/texture.cpp
    ${PROGRESSION_DIR}/src/renderer/graphics_api/texture.hpp
    ${PROGRESSION_DIR}/src/renderer/graphics_api/vertex_descriptor.cpp
    ${PROGRESSION_DIR}/src/renderer/graphics_api/vertex_descriptor.hpp
    
    ${PROGRESSION_DIR}/src/renderer/debug_marker.cpp
    ${PROGRESSION_DIR}/src/renderer/debug_marker.hpp
    ${PROGRESSION_DIR}/src/renderer/graphics_api.hpp
    ${PROGRESSION_DIR}/src/renderer/render_system.cpp
    ${PROGRESSION_DIR}/src/renderer/render_system.hpp
    ${PROGRESSION_DIR}/src/renderer/r_globals.hpp
    ${PROGRESSION_DIR}/src/renderer/r_init.cpp
    ${PROGRESSION_DIR}/src/renderer/r_init.hpp
    ${PROGRESSION_DIR}/src/renderer/vulkan.cpp
    ${PROGRESSION_DIR}/src/renderer/vulkan.hpp
    
    ${PROGRESSION_DIR}/src/utils/bitops.hpp
    ${PROGRESSION_DIR}/src/utils/cpu_profile.hpp
    ${PROGRESSION_DIR}/src/utils/file_dependency.cpp
    ${PROGRESSION_DIR}/src/utils/file_dependency.hpp
    ${PROGRESSION_DIR}/src/utils/filesystem.cpp
    ${PROGRESSION_DIR}/src/utils/filesystem.hpp
    ${PROGRESSION_DIR}/src/utils/json_parsing.cpp
    ${PROGRESSION_DIR}/src/utils/json_parsing.hpp
    ${PROGRESSION_DIR}/src/utils/logger.cpp
    ${PROGRESSION_DIR}/src/utils/logger.hpp
    ${PROGRESSION_DIR}/src/utils/noncopyable.hpp
    ${PROGRESSION_DIR}/src/utils/random.cpp
    ${PROGRESSION_DIR}/src/utils/random.hpp
    ${PROGRESSION_DIR}/src/utils/serializer.cpp
    ${PROGRESSION_DIR}/src/utils/serializer.hpp

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
)

set(ALL_FILES ${SRC} ${EXTERNALS})

if(UNIX AND NOT APPLE)
    set(SHADERC_LIB shaderc_combined)
    set(SPIRVCROSS_LIB spirv-cross-core)
elseif(MSVC)
    set(SHADERC_LIB ${Vulkan_INCLUDE_DIR}/../Lib/shaderc_combined.lib)
    set(SPIRVCROSS_LIB ${Vulkan_INCLUDE_DIR}/../Lib/spirv-cross-core.lib)
endif()

set(LIBS lz4 OpenMP::OpenMP_CXX ${SHADERC_LIB} ${SPIRVCROSS_LIB} glfw ${Vulkan_LIBRARIES})

include_directories(
    ${PROGRESSION_DIR}/src/
    ${PROGRESSION_DIR}/ext/
    ${Vulkan_INCLUDE_DIR}
    ${PROGRESSION_DIR}/ext/glfw/include
)

source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${ALL_FILES})

add_executable(${PROJECT_NAME} ${ALL_FILES})
SET_TARGET_POSTFIX(${PROJECT_NAME})
SET_TARGET_COMPILE_OPTIONS_DEFAULT(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} ${LIBS})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
