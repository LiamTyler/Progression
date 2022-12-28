set(EXT_DIR ${CMAKE_SOURCE_DIR}/code/external)

#----------------------------ASSIMP--------------------------------------------
if(NOT EXISTS ${CMAKE_BINARY_DIR}/lib/assimp-vc142-mt.lib)
	CONFIG_TIME_COMPILE(${EXT_DIR}/assimp ${CMAKE_BINARY_DIR}/ext/assimp Release)
	COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/assimp/bin/Release ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib assimp)
	COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/assimp/lib/Release ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib assimp)
endif()
set(ASSIMP_INCLUDES
	${PROGRESSION_DIR}/code/external/assimp/include
	${CMAKE_BINARY_DIR}/ext/assimp/include
	PARENT_SCOPE
)
set(ASSIMP_LIBS assimp-vc142-mt PARENT_SCOPE)

#------------------------------LZ4---------------------------------------------
if(NOT EXISTS ${CMAKE_BINARY_DIR}/lib/lz4.lib)
	CONFIG_TIME_COMPILE(${EXT_DIR}/lz4 ${CMAKE_BINARY_DIR}/ext/lz4 Release)
	COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/lz4/Release ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib lz4)
endif()
#------------------------------GLFW--------------------------------------------
if(NOT EXISTS ${CMAKE_BINARY_DIR}/lib/glfw3dll.lib)
	CONFIG_TIME_COMPILE(${EXT_DIR}/glfw ${CMAKE_BINARY_DIR}/ext/glfw Release)
	COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/glfw/src/Release ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib glfw)
endif()
set(GLFW_INCLUDES ${PROGRESSION_DIR}/code/external/glfw/include PARENT_SCOPE)
set(GLFW_LIBS glfw3dll PARENT_SCOPE)

#-------------------------------LUA--------------------------------------------
if(NOT EXISTS ${CMAKE_BINARY_DIR}/lib/lua.lib)
	CONFIG_TIME_COMPILE(${EXT_DIR}/lua ${CMAKE_BINARY_DIR}/ext/lua Release)
	COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/lua/Release ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib lua)
endif()
set(LUA_INCLUDES ${PROGRESSION_DIR}/code/external/lua/ PARENT_SCOPE)

#-------------------------------SHADERC + SPIRVCROSS--------------------------------------------
set(VULKAN_LIBS ${Vulkan_LIBRARIES} shaderc_combined spirv-cross-core PARENT_SCOPE)
if(MSVC)
	set(VULKAN_LIBS
		${Vulkan_LIBRARIES}
		${Vulkan_INCLUDE_DIR}/../Lib/shaderc_combined.lib
		${Vulkan_INCLUDE_DIR}/../Lib/spirv-cross-core.lib
		PARENT_SCOPE
	)
endif()

#-------------------------------BC7ENC--------------------------------------------
if(NOT EXISTS ${CMAKE_BINARY_DIR}/lib/bc7enc.lib)
	CONFIG_TIME_COMPILE(${EXT_DIR}/bc7enc_rdo ${CMAKE_BINARY_DIR}/ext/bc7enc Release)
	COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/bc7enc ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib bc7enc)
endif()

#-------------------------------LIBTIFF--------------------------------------------
if(NOT EXISTS ${CMAKE_BINARY_DIR}/lib/tiff.lib)
	CONFIG_TIME_COMPILE(${EXT_DIR}/libtiff ${CMAKE_BINARY_DIR}/ext/libtiff Release)
	COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/libtiff/libtiff/Release ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib tiff)
endif()

#-------------------------------IMAGELIB--------------------------------------------
# much more likely to be edited than the other projects, so always recompile
CONFIG_TIME_COMPILE_DEBUG_AND_RELEASE(${EXT_DIR}/ImageLib ${CMAKE_BINARY_DIR}/ext/ImageLib)
COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/ImageLib/lib ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib *)
COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/ImageLib/bin ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib *)
set(IMAGELIB_INCLUDES
    ${EXT_DIR}/ImageLib/
	${EXT_DIR}/libtiff/libtiff/
    ${CMAKE_BINARY_DIR}/ext/libtiff/libtiff/
    ${EXT_DIR}/compressonator/
    ${EXT_DIR}/compressonator/cmp_core/source/
	PARENT_SCOPE
)
set(IMAGELIB_LIBS ImageLib tiff bc7enc PARENT_SCOPE)
set(IMAGELIB_LIBS_DEBUG ImageLib_d tiff bc7enc PARENT_SCOPE)

set(IMAGELIB_EXT_FILES
	${EXT_DIR}/ImageLib/bc_compression.cpp
    ${EXT_DIR}/ImageLib/bc_compression.hpp
	${EXT_DIR}/ImageLib/bc_decompression.cpp
    ${EXT_DIR}/ImageLib/bc7_reference_tables.hpp
	${EXT_DIR}/ImageLib/image.cpp
    ${EXT_DIR}/ImageLib/image.hpp
	${EXT_DIR}/ImageLib/image_load.cpp
    ${EXT_DIR}/ImageLib/image_load.hpp
	${EXT_DIR}/ImageLib/image_save.cpp
    ${EXT_DIR}/ImageLib/image_save.hpp
	PARENT_SCOPE
)
