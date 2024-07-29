#pragma once

#include "core/pixel_formats.hpp"
#include "renderer/vulkan.hpp"
#include "shared/assert.hpp"
#include "shared/math_vec.hpp"
#include <vector>

// needs to be debug mode, since TaskGraph::Print relies on the Buffer::debugName and Texture::debugName,
//  which only exist in debug builds
#define TG_DEBUG USE_IF( USING( DEBUG_BUILD ) )
#define TG_STATS USE_IF( USING( DEVELOPMENT_BUILD ) )

#if USING( TG_DEBUG )
#define TG_DEBUG_ONLY( x ) x
#define TG_ASSERT( ... ) PG_ASSERT( __VA_ARGS__ )
#define TG_STAT( x ) x
#else // #if USING( TG_DEBUG )
#define TG_DEBUG_ONLY( x )
#define TG_ASSERT( ... )
#define TG_STAT( x )
#endif // #else // #if USING( TG_DEBUG )

namespace PG::Gfx
{

enum class ResourceType : u8
{
    NONE           = 0,
    TEXTURE        = ( 1 << 0 ),
    BUFFER         = ( 1 << 1 ),
    COLOR_ATTACH   = ( 1 << 2 ),
    DEPTH_ATTACH   = ( 1 << 3 ),
    STENCIL_ATTACH = ( 1 << 4 ),
};
PG_DEFINE_ENUM_OPS( ResourceType )

enum class ResourceState : u8
{
    UNUSED     = 0,
    READ       = 1,
    WRITE      = 2,
    READ_WRITE = 3,

    COUNT
};

using TGResourceHandle = u16;

template <typename T>
TGResourceHandle GetEmbeddedResHandle( T val )
{
    TGResourceHandle handle;
    memcpy( &handle, &val, sizeof( TGResourceHandle ) );
    return handle;
}

enum class TaskType : u16
{
    NONE     = 0, // only valid internally for signaling a resource has no previous task yet (first usage)
    COMPUTE  = 1,
    GRAPHICS = 2,
    TRANSFER = 3,
    PRESENT  = 4,
};

struct TaskHandle
{
    u16 index : 13;
    TaskType type : 3;

    TaskHandle( u16 inIndex, TaskType inType ) : index( inIndex ), type( inType ) {}
};

} // namespace PG::Gfx
