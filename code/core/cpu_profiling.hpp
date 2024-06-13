#pragma once

#include "core/feature_defines.hpp"

#if USING( PG_TRACY_PROFILING )

#include "tracy/Tracy.hpp"

#define PGP_FRAME_MARK FrameMark
#define PGP_ZONE_SCOPED ZoneScoped
#define PGP_ZONE_SCOPEDN( name ) ZoneScopedN( name )
#define PGP_ZONE_SCOPED_FMT( fmt, ... ) \
    ZoneScoped;                         \
    ZoneNameF( fmt, __VA_ARGS__ )

#else // #if USING( PG_TRACY_PROFILING )

// clang-format off
#define PGP_FRAME_MARK do {} while( 0 )
#define PGP_ZONE_SCOPED do {} while( 0 )
#define PGP_ZONE_SCOPEDN( name ) do {} while( 0 )
#define PGP_ZONE_SCOPED_FMT( fmt, ... ) do {} while( 0 )

// clang-format on

#endif // #else // #if USING( PG_TRACY_PROFILING )
