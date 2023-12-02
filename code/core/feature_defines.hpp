#pragma once

#include "shared/core_defines.hpp"
#include "shared/platform_defines.hpp"

#define PG_OPTIMIZE_SHADERS NOT_IN_USE
#define PG_RTX NOT_IN_USE

#if USING( SHIP_BUILD ) || !USING( GAME )
    #define ASSET_LIVE_UPDATE NOT_IN_USE
#else // #if USING( SHIP_BUILD ) || !USING( GAME )
    #define ASSET_LIVE_UPDATE IN_USE
#endif // #else // #if USING( SHIP_BUILD ) || !USING( GAME )

#if !USING( SHIP_BUILD )
    #define PG_GPU_PROFILING IN_USE
    #define PG_DEBUG_UI IN_USE
#else // #if !USING( SHIP_BUILD )
    #define PG_GPU_PROFILING NOT_IN_USE
    #define PG_DEBUG_UI NOT_IN_USE
#endif // #else // #if !USING( SHIP_BUILD )
