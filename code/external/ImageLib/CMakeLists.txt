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

CONFIG_TIME_COMPILE(${CMAKE_SOURCE_DIR}/ext/bc7enc_rdo ${CMAKE_BINARY_DIR}/ext/bc7enc Release)
COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/bc7enc ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib bc7enc)

CONFIG_TIME_COMPILE(${CMAKE_SOURCE_DIR}/ext/libtiff ${CMAKE_BINARY_DIR}/ext/libtiff Release)
COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/libtiff/libtiff/Release ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib tiff)

set(
	SRC
	${CMAKE_CURRENT_SOURCE_DIR}/src/bc_compression.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bc_compression.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/bc_decompression.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bc_decompression.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bc7_reference_tables.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/image.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/image.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/image_load.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/image_load.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/image_save.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/image_save.hpp
	
	${SHARED_DIR}/assert.hpp
	${SHARED_DIR}/core_defines.hpp
    ${SHARED_DIR}/filesystem.cpp
    ${SHARED_DIR}/filesystem.hpp
    ${SHARED_DIR}/float_conversions.hpp
    ${SHARED_DIR}/logger.cpp
    ${SHARED_DIR}/logger.hpp
    ${SHARED_DIR}/platform_defines.hpp
)

set(
	EXTERNALS
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/stb/stb_image.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/stb/stb_image_write.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/stb/stb_image_resize.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/tinyexr/tinyexr.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/tinyexr/tinyexr.cc
	
	${CMAKE_CURRENT_SOURCE_DIR}/ext/compressonator/cmp_core/shaders/bc6_encode_kernel.h
	${CMAKE_CURRENT_SOURCE_DIR}/ext/compressonator/cmp_core/shaders/bc6_encode_kernel.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ext/compressonator/cmp_core/shaders/bcn_common_kernel.h
	${CMAKE_CURRENT_SOURCE_DIR}/ext/compressonator/cmp_core/shaders/common_def.h
	${CMAKE_CURRENT_SOURCE_DIR}/ext/compressonator/cmp_core/source/cmp_core.h
	${CMAKE_CURRENT_SOURCE_DIR}/ext/compressonator/cmp_core/source/cmp_math_vec4.h
	${CMAKE_CURRENT_SOURCE_DIR}/ext/compressonator/cmp_core/source/cmp_math_func.h
)

set(SRC_ALL ${SRC} ${EXTERNALS})
#source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRC_ALL})

add_library(${PROJECT_NAME} ${SRC_ALL})
SET_TARGET_POSTFIX(${PROJECT_NAME})
SET_TARGET_COMPILE_OPTIONS_DEFAULT(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${ROOT_DIR}/code/
    ${ROOT_DIR}/code/external/
    ${CMAKE_CURRENT_SOURCE_DIR}/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/libtiff/libtiff/
    ${CMAKE_BINARY_DIR}/ext/libtiff/libtiff/
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/compressonator/
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/compressonator/cmp_core/source/
)
target_link_directories(${PROJECT_NAME} PRIVATE
	${CMAKE_BINARY_DIR}/lib/
)
target_link_libraries(${PROJECT_NAME} PRIVATE
	OpenMP::OpenMP_CXX tiff
)

