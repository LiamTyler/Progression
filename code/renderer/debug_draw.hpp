#pragma once

#include "core/bounding_box.hpp"
#include "renderer/graphics_api/command_buffer.hpp"

#define DEBUG_DRAW USE_IF( !USING( SHIP_BUILD ) )

namespace PG::Gfx::DebugDraw
{

enum class Color : u8
{
    BLACK,
    GRAY,
    WHITE,
    RED,
    GREEN,
    BLUE,
    YELLOW,
    MAGENTA,
    CYAN,

    COUNT
};

void Init();
void Shutdown();
void StartFrame();
void Draw( CommandBuffer& cmdBuf );

// all functions expect positions in world space
void AddLine( vec3 begin, vec3 end, Color color = Color::GREEN );
void AddAABB( const AABB& aabb, Color color = Color::GREEN );

} // namespace PG::Gfx::DebugDraw
