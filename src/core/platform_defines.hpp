#pragma once

/*
 * This is an AUTO GENERATED file from cmake/platform_defines.hpp.in. This will be overwritten whenever
 * cmake is run again, careful when editing.
 */

#include "core_defines.hpp"

#define PG_ROOT_DIR "/home/liam/Documents/ProgressionNew/"
#define PG_ASSET_DIR "/home/liam/Documents/ProgressionNew/assets/"
#define PG_BIN_DIR "/home/liam/Documents/ProgressionNew/build_debug/bin/"

#define LINUX_PROGRAM   IN_USE
#define WINDOWS_PROGRAM NOT_IN_USE
#define APPLE_PROGRAM   NOT_IN_USE


#ifdef CMAKE_DEFINE_DEBUG_BUILD
#define DEBUG_BUILD IN_USE
#else
#define DEBUG_BUILD NOT_IN_USE
#endif

#ifdef CMAKE_DEFINE_RELEASE_BUILD
#define RELEASE_BUILD IN_USE
#else
#define RELEASE_BUILD NOT_IN_USE
#endif

#ifdef CMAKE_DEFINE_SHIP_BUILD
#define SHIP_BUILD IN_USE
#else
#define SHIP_BUILD NOT_IN_USE
#endif

#ifdef CMAKE_DEFINE_COMPILING_CONVERTER
#define COMPILING_CONVERTER IN_USE
#else
#define COMPILING_CONVERTER NOT_IN_USE
#endif
