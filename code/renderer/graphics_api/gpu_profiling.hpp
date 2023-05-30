#pragma once

#include "core/feature_defines.hpp"
#include "renderer/graphics_api/command_buffer.hpp"
#include <string>

namespace PG::Gfx::Profile
{

    bool Init();
    void Shutdown();
    void DrawResultsOnScreen();

    void StartFrame( const CommandBuffer& cmdbuf );
    void Reset( const CommandBuffer& cmdbuf );
    void EndFrame();
    
    void StartTimestamp( const CommandBuffer& cmdbuf, const std::string& name );
    void EndTimestamp( const CommandBuffer& cmdbuf, const std::string& name );

} // namespace PG::Gfx::Profile

#if USING( PG_GPU_PROFILING )

#define PG_PROFILE_GPU_RESET( cmdbuf ) PG::Gfx::Profile::Reset( cmdbuf );
#define PG_PROFILE_GPU_START( cmdbuf, name ) PG::Gfx::Profile::StartTimestamp( cmdbuf, name );
#define PG_PROFILE_GPU_END( cmdbuf, name ) PG::Gfx::Profile::EndTimestamp( cmdbuf, name );

#else // #if USING( PG_GPU_PROFILING )

#define PG_PROFILE_GPU_RESET( cmdbuf ) do {} while( 0 )
#define PG_PROFILE_GPU_START( cmdbuf, name ) do {} while( 0 )
#define PG_PROFILE_GPU_END( cmdbuf, name ) do {} while( 0 )

#endif // #else // #if USING( PG_GPU_PROFILING )