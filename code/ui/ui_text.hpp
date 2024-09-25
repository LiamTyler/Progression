#pragma once

#include "asset/types/font.hpp"

namespace PG::UI::Text
{

enum class Justification : u8
{
    LEFT   = 0,
    CENTER = 1,
    RIGHT  = 2,

    NUM_JUSTIFICATION
};

void Init();
void Shutdown();

struct TextDrawInfo
{
    vec2 pos;
    vec4 color                  = vec4( 1, 1, 1, 1 );
    float fontSize              = 12;
    Justification justification = Justification::LEFT;
};

void Draw2D( const TextDrawInfo& textDrawInfo, const char* str );

} // namespace PG::UI::Text
