#pragma once

#include "renderer/graphics_api/command_buffer.hpp"
#include "core/platform_defines.hpp"
#include <string>

namespace PG
{
namespace Gfx
{
namespace Profile
{

    bool Init();
    void Shutdown();

    void Reset( const CommandBuffer& cmdbuf );
    void GetResults();
    
    void Timestamp( const CommandBuffer& cmdbuf, const std::string& name );
    uint64_t GetTimestamp( const std::string& name );
    float GetDuration( const std::string& start, const std::string& end );

} // namespace Profile
} // namespace Gfx
} // namespace PG

#if !USING( SHIP_BUILD )

#define PG_GPU_PROFILING IN_USE

#else // #if !USING( SHIP_BUILD )

#define PG_GPU_PROFILING NOT_IN_USE

#endif // #else // #if !USING( SHIP_BUILD )

#if USING( PG_GPU_PROFILING )

#define PG_PROFILE_GPU_RESET( cmdbuf ) PG::Gfx::Profile::Reset( cmdbuf );
#define PG_PROFILE_GPU_GET_RESULTS() PG::Gfx::Profile::GetResults();
#define PG_PROFILE_GPU_START( cmdbuf, name ) PG::Gfx::Profile::Timestamp( cmdbuf, name + std::string( "_Start" ) );
#define PG_PROFILE_GPU_END( cmdbuf, name ) PG::Gfx::Profile::Timestamp( cmdbuf, name + std::string( "_End" ) );

#else // #if USING( PG_GPU_PROFILING )

#define PG_PROFILE_GPU_RESET( cmdbuf ) do {} while( 0 )
#define PG_PROFILE_GPU_GET_RESULTS() do {} while( 0 )
#define PG_PROFILE_GPU_START( cmdbuf, name ) do {} while( 0 )
#define PG_PROFILE_GPU_END( cmdbuf, name ) do {} while( 0 )

#endif // #else // #if USING( PG_GPU_PROFILING )