#pragma once

#include "utils/logger.hpp"
#include <chrono>

#define CPU_PROFILER NOT_IN_USE
#if !USING( CPU_PROFILER )

#define PG_PROFILE_CPU_START( name ) do {} while( 0 )
#define PG_PROFILE_CPU_GET_DURATION( name ) do {} while( 0 )
#define PG_PROFILE_CPU_END( name ) do {} while( 0 )

#else // #if USING( CPU_PROFILER )

#define PG_PROFILE_CPU_START( name ) auto name ## __pgCpuProfile = std::chrono::system_clock::now()
#define PG_PROFILE_CPU_GET_DURATION( name ) (std::chrono::duration_cast< std::chrono::microseconds >( std::chrono::system_clock::now() - name ## __pgCpuProfile ).count() / 1000.0f)
#define PG_PROFILE_CPU_END( name ) LOG( "Profile '%s': %f\n", #name, PG_PROFILE_CPU_GET_DURATION( name ) )

#endif // #else // #if USING( CPU_PROFILER )
