#pragma once

#include "core/pixel_formats.hpp"
#include "renderer/vulkan.hpp"
#include "shared/assert.hpp"
#include "shared/math_vec.hpp"
#include <vector>

// needs to be debug mode, since TaskGraph::Print relies on the Buffer::debugName and Texture::debugName,
//  which only exist in debug builds
#define TG_DEBUG USE_IF( USING( DEBUG_BUILD ) )
#define TG_STATS USE_IF( !USING( SHIP_BUILD ) )

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

enum class ResourceType : uint8_t
{
    NONE           = 0,
    TEXTURE        = ( 1 << 0 ),
    BUFFER         = ( 1 << 1 ),
    COLOR_ATTACH   = ( 1 << 2 ),
    DEPTH_ATTACH   = ( 1 << 3 ),
    STENCIL_ATTACH = ( 1 << 4 ),
};
PG_DEFINE_ENUM_OPS( ResourceType )

enum class ResourceState : uint8_t
{
    READ       = 0,
    WRITE      = 1,
    READ_WRITE = 2,

    COUNT
};

using TGResourceHandle = uint16_t;

template <typename T>
TGResourceHandle GetEmbeddedResHandle( T val )
{
    TGResourceHandle handle;
    memcpy( &handle, &val, sizeof( TGResourceHandle ) );
    return handle;
}

} // namespace PG::Gfx
