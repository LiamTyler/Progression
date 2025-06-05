#pragma once

#include "asset/types/font.hpp"
#include "renderer/graphics_api/command_buffer.hpp"

namespace PG::UI::Text
{

void Init();
void Shutdown();

struct TextDrawInfo
{
    Font* font = nullptr; // if null, will use default font
    vec2 pos;
    vec4 color     = vec4( 1, 1, 1, 1 );
    float fontSize = 16;
};

vec2 MeasureText( const char* text, u32 length, Font* font, float fontSize );

void Draw2D( const TextDrawInfo& textDrawInfo, const char* str );

void Render( Gfx::CommandBuffer& cmdBuf );

// used by ui_system.cpp internally
void Clay_Draw2D( Gfx::CommandBuffer& cmdBuf, const std::string_view& str, const TextDrawInfo& textDrawInfo );
void Clay_FinalizeTextDraws();

} // namespace PG::UI::Text
