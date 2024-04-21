#pragma once

#include "shared/core_defines.hpp"
#include "shared/platform_defines.hpp"

#define PG_OPTIMIZE_SHADERS NOT_IN_USE
#define PG_RTX NOT_IN_USE // USE_IF( !USING( SHIP_BUILD ) )

#define ASSET_LIVE_UPDATE USE_IF( !USING( SHIP_BUILD ) && USING( GAME ) )
#define PG_GPU_PROFILING USE_IF( !USING( SHIP_BUILD ) )
#define PG_DEBUG_UI USE_IF( !USING( SHIP_BUILD ) )
