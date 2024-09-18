project(Engine LANGUAGES CXX C)

include(helpful_functions)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Profile;Ship")
SET_BIN_AND_LIB_DIRS(${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_PROFILE ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_PROFILE ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_PROFILE ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_SHIP ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_SHIP ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_SHIP ${CMAKE_BINARY_DIR}/lib)

if(UNIX AND NOT APPLE)
elseif(MSVC)
    # CMAKE_CXX_FLAGS_PROFILE are set by SET_TARGET_COMPILE_OPTIONS_DEFAULT()
    set(CMAKE_C_FLAGS_PROFILE "" CACHE STRING "" FORCE)
    set(CMAKE_EXE_LINKER_FLAGS_PROFILE "" CACHE STRING "" FORCE)
    set(CMAKE_SHARED_LINKER_FLAGS_PROFILE "" CACHE STRING "" FORCE)
    mark_as_advanced(CMAKE_CXX_FLAGS_PROFILE CMAKE_C_FLAGS_PROFILE CMAKE_EXE_LINKER_FLAGS_PROFILE CMAKE_SHARED_LINKER_FLAGS_PROFILE)
    
    # CMAKE_CXX_FLAGS_SHIP are set by SET_TARGET_COMPILE_OPTIONS_DEFAULT()
    set(CMAKE_C_FLAGS_SHIP "" CACHE STRING "" FORCE)
    set(CMAKE_EXE_LINKER_FLAGS_SHIP "" CACHE STRING "" FORCE)
    set(CMAKE_SHARED_LINKER_FLAGS_SHIP "" CACHE STRING "" FORCE)
    mark_as_advanced(CMAKE_CXX_FLAGS_SHIP CMAKE_C_FLAGS_SHIP CMAKE_EXE_LINKER_FLAGS_SHIP CMAKE_SHARED_LINKER_FLAGS_SHIP)
endif()

include(source_files)

set(CODE_DIR ${PROGRESSION_DIR}/code)
set(
	SRC
    
    ${ASSET_SRC}
	${UI_SRC}
	${ECS_SRC}
    
    ${CODE_DIR}/core/bounding_box.cpp
    ${CODE_DIR}/core/bounding_box.hpp
    ${CODE_DIR}/core/camera.cpp
    ${CODE_DIR}/core/camera.hpp
    ${CODE_DIR}/core/console_commands.cpp
    ${CODE_DIR}/core/console_commands.hpp
    ${CODE_DIR}/core/cpu_profiling.hpp
    ${CODE_DIR}/core/dvars.cpp
    ${CODE_DIR}/core/dvars.hpp
    ${CODE_DIR}/core/engine_globals.cpp
    ${CODE_DIR}/core/engine_globals.hpp
    ${CODE_DIR}/core/feature_defines.hpp
    ${CODE_DIR}/core/frustum.cpp
    ${CODE_DIR}/core/frustum.hpp
    ${CODE_DIR}/core/image_processing.cpp
    ${CODE_DIR}/core/image_processing.hpp
    ${CODE_DIR}/core/init.cpp
    ${CODE_DIR}/core/init.hpp
    ${CODE_DIR}/core/input.cpp
    ${CODE_DIR}/core/input.hpp
    ${CODE_DIR}/core/input_types.cpp
    ${CODE_DIR}/core/input_types.hpp
    ${CODE_DIR}/core/raw_input_types.cpp
    ${CODE_DIR}/core/raw_input_types.hpp
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
    ${CODE_DIR}/core/window.cpp
    ${CODE_DIR}/core/window.hpp
    
    ${CODE_DIR}/renderer/graphics_api/acceleration_structure.cpp
    ${CODE_DIR}/renderer/graphics_api/acceleration_structure.hpp
    ${CODE_DIR}/renderer/graphics_api/buffer.cpp
    ${CODE_DIR}/renderer/graphics_api/buffer.hpp
    ${CODE_DIR}/renderer/graphics_api/command_buffer.cpp
    ${CODE_DIR}/renderer/graphics_api/command_buffer.hpp
    ${CODE_DIR}/renderer/graphics_api/descriptor.cpp
    ${CODE_DIR}/renderer/graphics_api/descriptor.hpp
    ${CODE_DIR}/renderer/graphics_api/device.cpp
    ${CODE_DIR}/renderer/graphics_api/device.hpp
    ${CODE_DIR}/renderer/graphics_api/gpu_profiling.cpp
    ${CODE_DIR}/renderer/graphics_api/gpu_profiling.hpp
    ${CODE_DIR}/renderer/graphics_api/limits.hpp
    ${CODE_DIR}/renderer/graphics_api/pg_to_vulkan_types.hpp
    ${CODE_DIR}/renderer/graphics_api/physical_device.cpp
    ${CODE_DIR}/renderer/graphics_api/physical_device.hpp
    ${CODE_DIR}/renderer/graphics_api/pipeline.hpp
    ${CODE_DIR}/renderer/graphics_api/queue.hpp
    ${CODE_DIR}/renderer/graphics_api/sampler.cpp
    ${CODE_DIR}/renderer/graphics_api/sampler.hpp
    ${CODE_DIR}/renderer/graphics_api/swapchain.cpp
    ${CODE_DIR}/renderer/graphics_api/swapchain.hpp
    ${CODE_DIR}/renderer/graphics_api/synchronization.cpp
    ${CODE_DIR}/renderer/graphics_api/synchronization.hpp
    ${CODE_DIR}/renderer/graphics_api/texture.cpp
    ${CODE_DIR}/renderer/graphics_api/texture.hpp
    
    ${CODE_DIR}/renderer/brdf_functions.cpp
    ${CODE_DIR}/renderer/brdf_functions.hpp
    ${CODE_DIR}/renderer/debug_draw.cpp
    ${CODE_DIR}/renderer/debug_draw.hpp
    ${CODE_DIR}/renderer/debug_marker.cpp
    ${CODE_DIR}/renderer/debug_marker.hpp
    ${CODE_DIR}/renderer/debug_ui.cpp
    ${CODE_DIR}/renderer/debug_ui.hpp
    ${CODE_DIR}/renderer/debug_ui_console.cpp
    ${CODE_DIR}/renderer/debug_ui_console.hpp
    ${CODE_DIR}/renderer/graphics_api.hpp
    ${CODE_DIR}/renderer/render_system.cpp
    ${CODE_DIR}/renderer/render_system.hpp
    ${CODE_DIR}/renderer/r_bindless_manager.cpp
    ${CODE_DIR}/renderer/r_bindless_manager.hpp
    ${CODE_DIR}/renderer/r_dvars.cpp
    ${CODE_DIR}/renderer/r_dvars.hpp
    ${CODE_DIR}/renderer/r_globals.cpp
    ${CODE_DIR}/renderer/r_globals.hpp
    ${CODE_DIR}/renderer/r_gpu_upload_manager.cpp
    ${CODE_DIR}/renderer/r_gpu_upload_manager.hpp
    ${CODE_DIR}/renderer/r_init.cpp
    ${CODE_DIR}/renderer/r_init.hpp
    ${CODE_DIR}/renderer/r_pipeline_manager.cpp
    ${CODE_DIR}/renderer/r_pipeline_manager.hpp
    ${CODE_DIR}/renderer/r_sky.cpp
    ${CODE_DIR}/renderer/r_sky.hpp
	${CODE_DIR}/renderer/taskgraph/r_taskGraph.cpp
    ${CODE_DIR}/renderer/taskgraph/r_taskGraph.hpp
    ${CODE_DIR}/renderer/taskgraph/internal/r_tg_builder.cpp
    ${CODE_DIR}/renderer/taskgraph/internal/r_tg_builder.hpp
    ${CODE_DIR}/renderer/taskgraph/internal/r_tg_common.hpp
    ${CODE_DIR}/renderer/taskgraph/internal/r_tg_resource_packing.cpp
    ${CODE_DIR}/renderer/taskgraph/internal/r_tg_resource_packing.hpp
    ${CODE_DIR}/renderer/taskgraph/internal/r_tg_tasks.cpp
    ${CODE_DIR}/renderer/taskgraph/internal/r_tg_tasks.hpp
    ${CODE_DIR}/renderer/vulkan.cpp
    ${CODE_DIR}/renderer/vulkan.hpp
	
	${C_SHARED_SHADERS}
    
    ${PRIMARY_SHARED_SRC}
    ${CODE_DIR}/shared/float_conversions.hpp
    ${CODE_DIR}/shared/json_parsing.cpp
    ${CODE_DIR}/shared/json_parsing.hpp
    ${CODE_DIR}/shared/serializer.cpp
    ${CODE_DIR}/shared/serializer.hpp

    ${CMAKE_CURRENT_SOURCE_DIR}/remote_console_server.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/remote_console_server.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/engine_main.cpp
)

set(
	EXTERNALS
    ${CODE_DIR}/external/memory_map/MemoryMapped.h
    ${CODE_DIR}/external/memory_map/MemoryMapped.cpp
	
	${CODE_DIR}/external/vk-bootstrap/VkBootstrapDispatch.h
    ${CODE_DIR}/external/vk-bootstrap/VkBootstrap.cpp
    ${CODE_DIR}/external/vk-bootstrap/VkBootstrap.h
	${CODE_DIR}/external/vma/vk_mem_alloc.h
    ${CODE_DIR}/external/vma/vma_usage.cpp
    
    $<$<NOT:$<CONFIG:SHIP>>:
        ${CODE_DIR}/external/tracy/tracy/Tracy.hpp
        ${CODE_DIR}/external/tracy/tracy/TracyVulkan.hpp
        ${CODE_DIR}/external/tracy/TracyClient.cpp
    >
    
    $<$<NOT:$<CONFIG:PROFILE,SHIP>>:
        ${CODE_DIR}/external/imgui/imgui.h
        ${CODE_DIR}/external/imgui/imgui.cpp
        ${CODE_DIR}/external/imgui/imgui_demo.cpp
        ${CODE_DIR}/external/imgui/imgui_draw.cpp
        ${CODE_DIR}/external/imgui/imgui_tables.cpp
        ${CODE_DIR}/external/imgui/imgui_widgets.cpp
        $<IF:$<BOOL:${PG_USE_SDL}>,
            ${CODE_DIR}/external/imgui/backends/imgui_impl_sdl3.cpp
            ${CODE_DIR}/external/imgui/backends/imgui_impl_sdl3.h,
            ${CODE_DIR}/external/imgui/backends/imgui_impl_glfw.cpp
            ${CODE_DIR}/external/imgui/backends/imgui_impl_glfw.h
        >
        
        ${CODE_DIR}/external/imgui/backends/imgui_impl_vulkan.cpp
        ${CODE_DIR}/external/imgui/backends/imgui_impl_vulkan.h
    >
)

set(INTELLISENSE_ONLY ${IMAGELIB_EXT_FILES} ${GLFW_EXT_FILES} ${SDL_EXT_FILES})

set(ALL_FILES ${SRC} ${EXTERNALS} ${INTELLISENSE_ONLY} ${SHADERS})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${ALL_FILES})
set_source_files_properties(
    ${INTELLISENSE_ONLY}
    ${SHADERS}
    ${VULKAN_EXT_FILES}
    PROPERTIES HEADER_FILE_ONLY TRUE
)

add_executable(${PROJECT_NAME} ${ALL_FILES} ${VULKAN_EXT_FILES})
target_compile_definitions(${PROJECT_NAME} PUBLIC
    CMAKE_DEFINE_GAME
    $<IF:$<BOOL:${PG_USE_SDL}>,"PG_USE_SDL",${GLFW_COMPILE_DEFS}>
    $<$<NOT:$<CONFIG:SHIP>>: TRACY_ENABLE>
)
SET_TARGET_COMPILE_OPTIONS_DEFAULT(${PROJECT_NAME})
if(UNIX AND NOT APPLE)
    target_compile_options(${PROJECT_NAME} PUBLIC $<$<CONFIG:PROFILE,SHIP>: -O3>)
elseif(MSVC)
    target_compile_options(${PROJECT_NAME} PUBLIC $<$<CONFIG:PROFILE,SHIP>: /O2 /GL /Zi>)
    target_link_options(${PROJECT_NAME} PUBLIC $<$<CONFIG:PROFILE,SHIP>: /LTCG /INCREMENTAL:NO>)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
	DEBUG_POSTFIX _d
	RELEASE_POSTFIX _r
	PROFILE_POSTFIX _p
	SHIP_POSTFIX _s
)
target_include_directories(${PROJECT_NAME} PUBLIC
    ${COMMON_INCLUDE_DIRS}
    $<IF:$<BOOL:${PG_USE_SDL}>,${SDL_INCLUDES},${GLFW_INCLUDES}>
    ${IMAGELIB_INCLUDES}
    ${LUA_INCLUDES}
    ${Vulkan_INCLUDE_DIR}
    ${PROGRESSION_DIR}/assets/
    ${PROGRESSION_DIR}/assets/shaders/
    ${CODE_DIR}/external/imgui/
    ${CODE_DIR}/external/tracy/
)
target_link_libraries(${PROJECT_NAME} PUBLIC OpenMP::OpenMP_CXX ${VULKAN_LIBS} ${IMAGELIB_LIBS} lua lz4
    $<IF:$<BOOL:${PG_USE_SDL}>,${SDL_LIBS},${GLFW_LIBS}>
)
target_link_directories(${PROJECT_NAME} PUBLIC ${CMAKE_BINARY_DIR}/lib ${CMAKE_BINARY_DIR}/bin)
