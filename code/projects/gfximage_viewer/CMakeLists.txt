project(GfxImageViewer)

include(helpful_functions)
include(source_files)

set(SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/gfximage_viewer_main.cpp
    ${CODE_DIR}/asset/types/base_asset.hpp
    ${CODE_DIR}/asset/types/gfx_image.cpp
    ${CODE_DIR}/asset/types/gfx_image.hpp
    ${CODE_DIR}/asset/asset_cache.cpp
    ${CODE_DIR}/asset/asset_cache.hpp
    
	${CODE_DIR}/core/image_processing.cpp
    ${CODE_DIR}/core/image_processing.hpp
    ${CODE_DIR}/core/pixel_formats.cpp
    ${CODE_DIR}/core/pixel_formats.hpp
    
    ${PRIMARY_SHARED_SRC}
    ${CODE_DIR}/shared/serializer.cpp
    ${CODE_DIR}/shared/serializer.hpp
)

set(
	EXTERNALS
    ${CODE_DIR}/external/memory_map/MemoryMapped.h
    ${CODE_DIR}/external/memory_map/MemoryMapped.cpp
)

set(ALL_FILES ${SRC} ${EXTERNALS} ${IMAGELIB_EXT_FILES})
set_source_files_properties(${IMAGELIB_EXT_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${ALL_FILES})


add_executable(${PROJECT_NAME} ${ALL_FILES})
SET_TARGET_POSTFIX(${PROJECT_NAME})
SET_TARGET_COMPILE_OPTIONS_DEFAULT(${PROJECT_NAME})
target_compile_definitions(${PROJECT_NAME} PUBLIC CMAKE_DEFINE_GFXIMAGE_VIEWER)

set(LIBS OpenMP::OpenMP_CXX ${VULKAN_LIBS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBS}
    debug ${IMAGELIB_LIBS_DEBUG} 
    optimized ${IMAGELIB_LIBS}
)
target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/lib ${CMAKE_BINARY_DIR}/bin)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${COMMON_INCLUDE_DIRS}
    ${Vulkan_INCLUDE_DIR}
    ${IMAGELIB_INCLUDES}
)
