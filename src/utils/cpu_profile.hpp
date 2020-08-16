#pragma once

#include "utils/logger.hpp"
#include <chrono>

#if USING( SHIP_BUILD )

#define PG_PROFILE_START( name ) do {} while( 0 )
#define PG_PROFILE_END( name ) do {} while( 0 )

#else // #if USING( SHIP_BUILD )

#define PG_PROFILE_START( name ) auto name ## __pgCpuProfile = std::chrono::system_clock::now()
#define PG_PROFILE_GET_DURATION( name ) (std::chrono::duration_cast< std::chrono::microseconds >( std::chrono::system_clock::now() - name ## __pgCpuProfile ).count() / 1000.0f)
#define PG_PROFILE_END( name ) LOG( "Profile '%s': %f\n", #name, PG_PROFILE_GET_DURATION( name ) )

#endif // #else // #if USING( SHIP_BUILD )
