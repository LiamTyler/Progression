#pragma once

#include "asset/types/pipeline.hpp"
#include "core/pixel_formats.hpp"
#include "renderer/vulkan.hpp"
#include "shared/math_vec.hpp"

namespace PG
{

namespace Gfx
{

struct Viewport
{
    Viewport() = default;
    Viewport( f32 w, f32 h ) : width( w ), height( h ) {}

    f32 x        = 0;
    f32 y        = 0;
    f32 width    = 0;
    f32 height   = 0;
    f32 minDepth = 0.0f;
    f32 maxDepth = 1.0f;
};

struct Scissor
{
    Scissor() = default;
    Scissor( i32 inX, i32 inY, i32 w, i32 h ) : x( inX ), y( inY ), width( w ), height( h ) {}

    i32 x      = 0;
    i32 y      = 0;
    i32 width  = 0;
    i32 height = 0;
};

} // namespace Gfx
} // namespace PG
