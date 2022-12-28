cmake_minimum_required (VERSION 3.7.2)
Project(ImageLib)

if(NOT PROGRESSION_DIR)
    set(ROOT_DIR ${CMAKE_SOURCE_DIR}/../../../)
else()
    set(ROOT_DIR ${PROGRESSION_DIR})
endif()
list(APPEND CMAKE_MODULE_PATH "${ROOT_DIR}/cmake")
set(SHARED_DIR ${ROOT_DIR}/code/shared/)
include(helpful_functions)

set(CMAKE_SUPPRESS_REGENERATION true)
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)
find_package(OpenMP REQUIRED)

SET_BIN_AND_LIB_DIRS(${CMAKE_BINARY_DIR}/lib ${CMAKE_BINARY_DIR}/lib)

set(
	SRC
	${CMAKE_CURRENT_SOURCE_DIR}/bc_compression.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bc_compression.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/bc_decompression.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bc7_reference_tables.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/image.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/image.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/image_load.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/image_load.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/image_save.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/image_save.hpp
	
	${SHARED_DIR}/assert.hpp
	${SHARED_DIR}/core_defines.hpp
    ${SHARED_DIR}/filesystem.cpp
    ${SHARED_DIR}/filesystem.hpp
    ${SHARED_DIR}/float_conversions.hpp
    ${SHARED_DIR}/logger.cpp
    ${SHARED_DIR}/logger.hpp
    ${SHARED_DIR}/platform_defines.hpp
)

set(EXT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(
	EXTERNALS
    ${EXT_DIR}/stb/stb_image.h
    ${EXT_DIR}/stb/stb_image_write.h
    ${EXT_DIR}/stb/stb_image_resize.h
    ${EXT_DIR}/tinyexr/tinyexr.h
    ${EXT_DIR}/tinyexr/tinyexr.cc
	
	${EXT_DIR}/compressonator/cmp_core/shaders/bc6_encode_kernel.h
	${EXT_DIR}/compressonator/cmp_core/shaders/bc6_encode_kernel.cpp
	${EXT_DIR}/compressonator/cmp_core/shaders/bcn_common_kernel.h
	${EXT_DIR}/compressonator/cmp_core/shaders/common_def.h
	${EXT_DIR}/compressonator/cmp_core/source/cmp_core.h
	${EXT_DIR}/compressonator/cmp_core/source/cmp_math_vec4.h
	${EXT_DIR}/compressonator/cmp_core/source/cmp_math_func.h
)

set(SRC_ALL ${SRC} ${EXTERNALS})
#source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRC_ALL})

add_library(${PROJECT_NAME} ${SRC_ALL})
SET_TARGET_POSTFIX(${PROJECT_NAME})
SET_TARGET_COMPILE_OPTIONS_DEFAULT(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${ROOT_DIR}/code/
    ${ROOT_DIR}/code/external/
    ${EXT_DIR}/libtiff/libtiff/
    ${ROOT_DIR}/build/ext/libtiff/libtiff/
    ${EXT_DIR}/compressonator/
    ${EXT_DIR}/compressonator/cmp_core/source/
)
target_link_directories(${PROJECT_NAME} PUBLIC ${CMAKE_BINARY_DIR}/lib/)
target_link_libraries(${PROJECT_NAME} PUBLIC OpenMP::OpenMP_CXX tiff)

