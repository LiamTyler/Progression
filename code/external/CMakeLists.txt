set(EXT_DIR ${CMAKE_SOURCE_DIR}/code/external)

#----------------------------ASSIMP--------------------------------------------
set(ASSIMP_CONFIG_ARGS
    -DASSIMP_BUILD_TESTS=OFF
    -DASSIMP_INSTALL=OFF
    -DASSIMP_NO_EXPORT=ON
    -DASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT=OFF
    -DASSIMP_BUILD_FBX_IMPORTER=ON
    -DASSIMP_BUILD_3DS_IMPORTER=ON
    -DASSIMP_BUILD_PLY_IMPORTER=ON
    -DASSIMP_BUILD_COLLADA_IMPORTER=ON
    -DASSIMP_BUILD_STL_IMPORTER=ON
    -DASSIMP_BUILD_GLTF_IMPORTER=ON
    -DASSIMP_BUILD_OBJ_IMPORTER=ON
)
if(MSVC)
    set(ASSIMP_LIB assimp-vc${MSVC_TOOLSET_VERSION}-mt.dll)
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/bin/${ASSIMP_LIB})
        CONFIG_TIME_COMPILE(${EXT_DIR}/assimp ${CMAKE_BINARY_DIR}/ext/assimp Release ${ASSIMP_CONFIG_ARGS})
        COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/assimp/bin/Release ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib assimp)
        COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/assimp/lib/Release ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib assimp)
    endif()
    set(ASSIMP_LIBS ${ASSIMP_LIB} PARENT_SCOPE)
else()
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/bin/libassimp.so)
        CONFIG_TIME_COMPILE(${EXT_DIR}/assimp ${CMAKE_BINARY_DIR}/ext/assimp Release ${ASSIMP_CONFIG_ARGS})
        COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/assimp/bin/ ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib assimp)
    endif()
    set(ASSIMP_LIBS assimp PARENT_SCOPE)
endif()
set(ASSIMP_INCLUDES
	${PROGRESSION_DIR}/code/external/assimp/include
	${CMAKE_BINARY_DIR}/ext/assimp/include
	PARENT_SCOPE
)
##this isn't needed for some reason?
#if(WIN32)
#    SET_LOCAL_AND_PARENT(ASSIMP_COMPILE_DEFS "ASSIMP_DLL")
#endif()

#------------------------------LZ4---------------------------------------------
if(MSVC)
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/lib/lz4.lib)
        CONFIG_TIME_COMPILE(${EXT_DIR}/lz4 ${CMAKE_BINARY_DIR}/ext/lz4 Release)
        COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/lz4/Release ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib lz4)
    endif()
else()
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/lib/liblz4.a)
        CONFIG_TIME_COMPILE(${EXT_DIR}/lz4 ${CMAKE_BINARY_DIR}/ext/lz4 Release)
        COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/lz4/ ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib lz4)
    endif()
endif()

#------------------------------GLFW--------------------------------------------
set(GLFW_CONFIG_ARGS
    -DBUILD_SHARED_LIBS=ON
    -DGLFW_BUILD_EXAMPLES=OFF
    -DGLFW_BUILD_TESTS=OFF 
    -DGLFW_BUILD_DOCS=OFF
    -DGLFW_INSTALL=OFF
)
if(MSVC)
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/lib/glfw3dll.lib)
        CONFIG_TIME_COMPILE(${EXT_DIR}/glfw ${CMAKE_BINARY_DIR}/ext/glfw Release ${GLFW_CONFIG_ARGS})
        COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/glfw/src/Release ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib glfw)
    endif()
    set(GLFW_LIBS glfw3dll PARENT_SCOPE)
else()
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/bin/libglfw.so)
        CONFIG_TIME_COMPILE(${EXT_DIR}/glfw ${CMAKE_BINARY_DIR}/ext/glfw Release ${GLFW_CONFIG_ARGS})
        COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/glfw/src ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib glfw)
    endif()
    set(GLFW_LIBS glfw PARENT_SCOPE)
endif()

set(GLFW_INCLUDES ${EXT_DIR}/glfw/include PARENT_SCOPE)
SET_LOCAL_AND_PARENT(GLFW_EXT_FILES ${EXT_DIR}/glfw/include/GLFW/glfw3.h)
if(WIN32)
    SET_LOCAL_AND_PARENT(GLFW_COMPILE_DEFS "GLFW_DLL")
endif()

#------------------------------SDL3--------------------------------------------
set(SDL_CONFIG_ARGS -DSDL_TEST_LIBRARY=OFF)
if(MSVC)
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/lib/SDL3.lib)
        CONFIG_TIME_COMPILE(${EXT_DIR}/sdl3 ${CMAKE_BINARY_DIR}/ext/sdl3 Release ${SDL_CONFIG_ARGS})
        COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/sdl3/Release ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib SDL3)
    endif()
    set(SDL_LIBS SDL3 PARENT_SCOPE)
else()
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/bin/libSDL3.so)
        CONFIG_TIME_COMPILE(${EXT_DIR}/sdl3 ${CMAKE_BINARY_DIR}/ext/sdl3 Release ${SDL_CONFIG_ARGS})
        COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/sdl3 ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib SDL3)
    endif()
    set(SDL_LIBS SDL3 PARENT_SCOPE)
endif()

set(SDL_INCLUDES ${EXT_DIR}/sdl3/include PARENT_SCOPE)
file(GLOB tmpSDLFiles ${EXT_DIR}/sdl3/include/SDL3/*)
SET_LOCAL_AND_PARENT(SDL_EXT_FILES ${tmpSDLFiles})

#-------------------------------LUA--------------------------------------------
if(MSVC)
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/lib/lua.lib)
        CONFIG_TIME_COMPILE(${EXT_DIR}/lua ${CMAKE_BINARY_DIR}/ext/lua Release)
        COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/lua/Release ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib lua)
    endif()
else()
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/lib/liblua.a)
        CONFIG_TIME_COMPILE(${EXT_DIR}/lua ${CMAKE_BINARY_DIR}/ext/lua Release)
        COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/lua ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib lua)
    endif()
endif()
set(LUA_INCLUDES ${PROGRESSION_DIR}/code/external/lua/ PARENT_SCOPE)

#-------------------------------SHADERC + SPIRVCROSS---------------------------
if(MSVC)
	set(VULKAN_LIBS
		${Vulkan_LIBRARIES}
		${Vulkan_INCLUDE_DIR}/../Lib/shaderc_combined.lib
		${Vulkan_INCLUDE_DIR}/../Lib/spirv-cross-core.lib
		PARENT_SCOPE
	)
    file(GLOB tmpFiles ${Vulkan_INCLUDE_DIR}/shaderc/* ${Vulkan_INCLUDE_DIR}/spirv_cross/*)
    SET_LOCAL_AND_PARENT(VULKAN_EXT_FILES ${tmpFiles})
else()
    set(VULKAN_LIBS
        ${Vulkan_LIBRARIES}
        ${Vulkan_INCLUDE_DIR}/../lib/libshaderc_combined.a
        ${Vulkan_INCLUDE_DIR}/../lib/libspirv-cross-core.a
        PARENT_SCOPE
    )
    file(GLOB tmpFiles ${Vulkan_INCLUDE_DIR}/shaderc/* ${Vulkan_INCLUDE_DIR}/spirv_cross/*)
    SET_LOCAL_AND_PARENT(VULKAN_EXT_FILES ${tmpFiles})
endif()

#-------------------------------BC7ENC-----------------------------------------
if(MSVC)
    #if(NOT EXISTS ${CMAKE_BINARY_DIR}/lib/bc7enc.lib)
        CONFIG_TIME_COMPILE(${EXT_DIR}/bc7enc_rdo ${CMAKE_BINARY_DIR}/ext/bc7enc Release)
        COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/bc7enc ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib bc7enc)
    #endif()
else()
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/lib/libbc7enc.a)
        CONFIG_TIME_COMPILE(${EXT_DIR}/bc7enc_rdo ${CMAKE_BINARY_DIR}/ext/bc7enc Release)
        COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/bc7enc ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib bc7enc)
    endif()
endif()

#-------------------------------LIBTIFF----------------------------------------
if(MSVC)
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/lib/tiff.lib)
        CONFIG_TIME_COMPILE(${EXT_DIR}/libtiff ${CMAKE_BINARY_DIR}/ext/libtiff Release)
        COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/libtiff/libtiff/Release ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib tiff)
    endif()
else()
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/bin/libtiff.so)
        CONFIG_TIME_COMPILE(${EXT_DIR}/libtiff ${CMAKE_BINARY_DIR}/ext/libtiff Release)
        COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/libtiff/libtiff ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib tiff)
    endif()
endif()

#-------------------------------IMAGELIB---------------------------------------
# much more likely to be edited than the other projects, so always recompile
CONFIG_TIME_COMPILE_DEBUG_AND_RELEASE(${EXT_DIR}/ImageLib ${CMAKE_BINARY_DIR}/ext/ImageLib)
if(MSVC)
    COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/ImageLib/lib ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib *)
    COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/ImageLib/bin ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib *)
else()
    COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/ImageLib/Debug/lib ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib *)
    COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/ImageLib/Release/lib ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib *)
endif()
set(IMAGELIB_INCLUDES
    ${EXT_DIR}/ImageLib/
	${EXT_DIR}/libtiff/libtiff/
    ${CMAKE_BINARY_DIR}/ext/libtiff/libtiff/
    ${EXT_DIR}/compressonator/
    ${EXT_DIR}/compressonator/cmp_core/source/
	PARENT_SCOPE
)
# NOTE: need to do this for all libraries libtiff ends up using (so far only zlib on linux?)
set(ZLIB_LIBS "")
if(NOT MSVC)
    find_package(ZLIB)
    if(ZLIB_FOUND)
        set(ZLIB_LIBS ${ZLIB_LIBRARIES})
    endif()
endif()
set(IMAGELIB_LIBS ImageLib tiff bc7enc ${ZLIB_LIBS} PARENT_SCOPE)
set(IMAGELIB_LIBS_DEBUG ImageLib_d tiff bc7enc ${ZLIB_LIBS} PARENT_SCOPE)

SET_LOCAL_AND_PARENT(IMAGELIB_EXT_FILES
	${EXT_DIR}/ImageLib/bc_compression.cpp
    ${EXT_DIR}/ImageLib/bc_compression.hpp
	${EXT_DIR}/ImageLib/bc_decompression.cpp
    ${EXT_DIR}/ImageLib/bc7_reference_tables.hpp
	${EXT_DIR}/ImageLib/image.cpp
    ${EXT_DIR}/ImageLib/image.hpp
	${EXT_DIR}/ImageLib/image_load.cpp
	${EXT_DIR}/ImageLib/image_save.cpp
    ${EXT_DIR}/ImageLib/image_transformations.cpp
	${EXT_DIR}/ImageLib/image_transformations.hpp
)

#-------------------------------PUGIXML----------------------------------------
# Actually using the header only, not compiling the library
set(PUGIXML_INCLUDES ${EXT_DIR}/pugixml-1.13/src/ PARENT_SCOPE)
file(GLOB tmpFiles ${EXT_DIR}/pugixml-1.13/src/*)
SET_LOCAL_AND_PARENT(PUGIXML_EXT_FILES ${tmpFiles})

#-------------------------------FREETYPE2--------------------------------------
if(MSVC)
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/lib/freetype.lib)
        CONFIG_TIME_COMPILE(${EXT_DIR}/freetype-2.13.2 ${CMAKE_BINARY_DIR}/ext/freetype Release)
        COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/freetype/Release ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib freetype)
    endif()
else()
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/lib/libfreetype.a)
        CONFIG_TIME_COMPILE(${EXT_DIR}/freetype-2.13.2 ${CMAKE_BINARY_DIR}/ext/freetype Release)
        COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/freetype ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib freetype)
    endif()
endif()
set(FREETYPE_INCLUDES ${EXT_DIR}/freetype-2.13.2/include/ PARENT_SCOPE)
if (WIN32)
    set(FREETYPE_LIBS freetype PARENT_SCOPE)
else()
    set(FREETYPE_LIBS freetype bz2 PARENT_SCOPE)
endif()

#-------------------------------MSDFGEN----------------------------------------
if(MSVC)
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/lib/msdfgen-core.lib)
        CONFIG_TIME_COMPILE(${EXT_DIR}/msdfgen ${CMAKE_BINARY_DIR}/ext/msdfgen Release)
        COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/msdfgen/Release ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib msdfgen)
    endif()
else()
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/bin/libmsdfgen-core.so)
        CONFIG_TIME_COMPILE(${EXT_DIR}/msdfgen ${CMAKE_BINARY_DIR}/ext/msdfgen Release)
        COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/msdfgen ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib msdfgen)
    endif()
endif()
set(MSDFGEN_INCLUDES ${EXT_DIR}/msdfgen/ PARENT_SCOPE)
set(MSDFGEN_LIBS msdfgen-core PARENT_SCOPE)
file(GLOB tmpFiles ${EXT_DIR}/msdfgen/core/*)
SET_LOCAL_AND_PARENT(MSDFGEN_EXT_FILES ${tmpFiles})
if(WIN32)
    SET_LOCAL_AND_PARENT(MSDFGEN_COMPILE_DEFS "MSDFGEN_PUBLIC=__declspec(dllimport)")
endif()

#-------------------------------ISOCLINE---------------------------------------
if(MSVC)
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/lib/isocline.lib)
        CONFIG_TIME_COMPILE(${EXT_DIR}/isocline ${CMAKE_BINARY_DIR}/ext/isocline Release)
        COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/isocline/Release ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib isocline)
    endif()
else()
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/lib/libisocline.a)
        CONFIG_TIME_COMPILE(${EXT_DIR}/isocline ${CMAKE_BINARY_DIR}/ext/isocline Release)
        COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/isocline ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib isocline)
    endif()
endif()
set(ISOCLINE_INCLUDES ${EXT_DIR}/isocline/ PARENT_SCOPE)
set(ISOCLINE_LIBS isocline PARENT_SCOPE)
file(GLOB tmpFiles ${EXT_DIR}/isocline/src/* ${EXT_DIR}/isocline/include/*)
SET_LOCAL_AND_PARENT(ISOCLINE_EXT_FILES ${tmpFiles})

#-------------------------------MESHOPTIMIZER----------------------------------
if(MSVC)
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/lib/meshoptimizer.lib)
        CONFIG_TIME_COMPILE(${EXT_DIR}/meshoptimizer ${CMAKE_BINARY_DIR}/ext/meshoptimizer Release)
        COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/meshoptimizer/Release ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib meshoptimizer)
    endif()
else()
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/lib/libmeshoptimizer.a)
        CONFIG_TIME_COMPILE(${EXT_DIR}/meshoptimizer ${CMAKE_BINARY_DIR}/ext/meshoptimizer Release)
        COPY_BUILD_FILES(${CMAKE_BINARY_DIR}/ext/meshoptimizer ${CMAKE_BINARY_DIR}/bin ${CMAKE_BINARY_DIR}/lib meshoptimizer)
    endif()
endif()
#set(MESHOPT_INCLUDES ${EXT_DIR}/meshoptimizer/ PARENT_SCOPE)
set(MESHOPT_LIBS meshoptimizer PARENT_SCOPE)
file(GLOB tmpFiles ${EXT_DIR}/meshoptimizer/src/*)
SET_LOCAL_AND_PARENT(MESHOPT_EXT_FILES ${tmpFiles})
