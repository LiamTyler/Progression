#pragma once

#include "core/bounding_box.hpp"
#include "core/frustum.hpp"
#include "renderer/graphics_api/command_buffer.hpp"

#define DEBUG_DRAW USE_IF( USING( DEVELOPMENT_BUILD ) )

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
void Draw2D( CommandBuffer& cmdBuf );
void Draw3D( CommandBuffer& cmdBuf );

// all functions expect positions in world space
void AddLine( vec3 begin, vec3 end, Color color = Color::GREEN );
void AddAABB( const AABB& aabb, Color color = Color::GREEN );
void AddFrustum( const Frustum& frustum, Color color = Color::GREEN );
void AddText2D( vec2 normalizedPos, Color color, const char* str );

} // namespace PG::Gfx::DebugDraw
