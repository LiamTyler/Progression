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
    Viewport( float w, float h ) : width( w ), height( h ) {}

    float x        = 0;
    float y        = 0;
    float width    = 0;
    float height   = 0;
    float minDepth = 0.0f;
    float maxDepth = 1.0f;
};

struct Scissor
{
    Scissor() = default;
    Scissor( int w, int h ) : width( w ), height( h ) {}

    int x      = 0;
    int y      = 0;
    int width  = 0;
    int height = 0;
};

} // namespace Gfx
} // namespace PG
