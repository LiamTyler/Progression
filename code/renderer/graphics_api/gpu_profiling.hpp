#pragma once

#include "core/feature_defines.hpp"
#include "renderer/graphics_api/command_buffer.hpp"
#include <string>

namespace PG::Gfx::Profile
{

struct QueryRecord
{
    u16 profileEntryIndex;
    u16 startQuery;
    u16 endQuery;
};

void Init();
void Shutdown();
void DrawResultsOnScreen();

void StartFrame( const CommandBuffer& cmdbuf );

u16 GetOrCreateProfileEntry( const std::string& name );
QueryRecord& StartTimestamp( const CommandBuffer& cmdbuf, u16 profileEntryIdx );
void EndTimestamp( const CommandBuffer& cmdbuf, QueryRecord& record );

} // namespace PG::Gfx::Profile

#if USING( PG_GPU_PROFILING )

#define PG_PROFILE_GPU_START_FRAME( cmdbuf ) PG::Gfx::Profile::StartFrame( cmdbuf );
#define PG_PROFILE_GPU_START( cmdbuf, identifier, name )                                            \
    u16 __pg__gfx__profile__##identifier##_idx = PG::Gfx::Profile::GetOrCreateProfileEntry( name ); \
    PG::Gfx::Profile::QueryRecord& __pg__gfx__profile__##identifier##_entry =                       \
        PG::Gfx::Profile::StartTimestamp( cmdbuf, __pg__gfx__profile__##identifier##_idx );
#define PG_PROFILE_GPU_END( cmdbuf, identifier ) PG::Gfx::Profile::EndTimestamp( cmdbuf, __pg__gfx__profile__##identifier##_entry );

#else // #if USING( PG_GPU_PROFILING )

#define PG_PROFILE_GPU_START_FRAME( cmdbuf ) \
    do                                       \
    {                                        \
    } while ( 0 )
#define PG_PROFILE_GPU_START( cmdbuf, identifier, name ) \
    do                                                   \
    {                                                    \
    } while ( 0 )
#define PG_PROFILE_GPU_END( cmdbuf, identifier ) \
    do                                           \
    {                                            \
    } while ( 0 )

#endif // #else // #if USING( PG_GPU_PROFILING )
