cmake_minimum_required (VERSION 3.0)

project(Converter LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LINUX_PROGRAM   "NOT_IN_USE")
set(WINDOWS_PROGRAM "NOT_IN_USE")
set(APPLE_PROGRAM   "NOT_IN_USE")

if(UNIX AND NOT APPLE)
    set(LINUX_PROGRAM   "IN_USE")
elseif(WIN32)
    set(WINDOWS_PROGRAM "IN_USE")
elseif(APPLE)
    set(APPLE_PROGRAM   "IN_USE")
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Ship")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_SHIP ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_SHIP ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_SHIP ${CMAKE_BINARY_DIR}/lib)

if(UNIX AND NOT APPLE)
    #set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Werror -Wextra -Wno-implicit-fallthrough -Wshadow -Wno-unused-variable -Wno-unused-function")
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wno-unused-function")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
    set(CMAKE_CXX_FLAGS_SHIP "${CMAKE_CXX_FLAGS_SHIP} -O3") # look up Ofast
elseif(MSVC)
    #set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /WX")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
    set(CMAKE_CXX_FLAGS_SHIP "${CMAKE_CXX_FLAGS_SHIP} /O2")
endif()

if(MSVC)
    SET(CMAKE_CXX_FLAGS_SHIP
        "${CMAKE_CXX_FLAGS_SHIP} /O2"
        CACHE STRING "Flags used by the C++ compiler during coverage builds."
        FORCE
    )
    SET(CMAKE_C_FLAGS_SHIP
        ""
        CACHE STRING "Flags used by the C compiler during coverage builds."
        FORCE
    )
    SET(CMAKE_EXE_LINKER_FLAGS_SHIP
        ""
        CACHE STRING "Flags used for linking binaries during coverage builds."
        FORCE
    )
    SET(CMAKE_SHARED_LINKER_FLAGS_SHIP
        ""
        CACHE STRING "Flags used by the shared libraries linker during coverage builds."
        FORCE
    )
    MARK_AS_ADVANCED(
        CMAKE_CXX_FLAGS_SHIP
        CMAKE_C_FLAGS_SHIP
        CMAKE_EXE_LINKER_FLAGS_SHIP
        CMAKE_SHARED_LINKER_FLAGS_SHIP
    )
endif()

set(LIBS)
find_package(OpenMP)
if(NOT OpenMP_CXX_FOUND)
    message("OpenMP not found! Rendering will be single threaded")
else()
    set(LIBS ${LIBS} OpenMP::OpenMP_CXX)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(NOT PROGRESSION_DIR)
    set(PROGRESSION_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()

configure_file(${PROGRESSION_DIR}/cmake/platform_defines.hpp.in ${PROGRESSION_DIR}/src/platform_defines.hpp)

set(
	SRC
	src/assert.hpp
	src/image.cpp
    src/image.hpp
    src/gfx_image.cpp
    src/gfx_image.hpp
    src/pixel_formats.cpp
    src/pixel_formats.hpp
    src/resource.hpp
    src/core_defines.hpp
    src/platform_defines.hpp
    
    src/utils/logger.hpp
    src/utils/logger.cpp
    src/utils/filesystem.hpp
    src/utils/filesystem.cpp
    src/utils/cpu_profile.hpp
    
    src/main.cpp
)


set(
	EXTERNALS
    ext/stb/stb_image.h
    ext/stb/stb_image_write.h
    ext/stb/stb_image_resize.h
    ext/tinyexr/tinyexr.h
    ext/tinyexr/tinyexr.cc
)

set(
	ALL_FILES
	${SRC}
	${EXTERNALS}
)

include_directories(
    ${CMAKE_SOURCE_DIR}/src/
    ${CMAKE_SOURCE_DIR}/ext/
)

SET(listVar "")
FOREACH(f ${ALL_FILES})
    LIST(APPEND listVar "${CMAKE_CURRENT_SOURCE_DIR}/${f}")
ENDFOREACH(f)
set(ALL_FILES ${listVar})

source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${ALL_FILES})

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_executable(Converter ${ALL_FILES})

if(MSVC)
    target_compile_options(Converter PRIVATE "/MP")
endif()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Converter)

# link in the static libs
target_link_libraries(Converter ${LIBS})
