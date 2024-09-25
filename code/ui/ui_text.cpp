#include "ui/ui_text.hpp"
#include "asset/asset_manager.hpp"
#include "renderer/r_globals.hpp"
#include "renderer/r_pipeline_manager.hpp"

namespace PG::UI::Text
{

static Pipeline* s_text2DPipeline;
static const Font* s_font;

struct LocalFrameData
{
    Gfx::Buffer textVB;
    u32 numTextCharacters;
};

static LocalFrameData s_localFrameDatas[NUM_FRAME_OVERLAP];
static Pipeline* s_debugTextPipeline;
static GfxImage* s_debugTextFontAtlas;

inline LocalFrameData& LocalData() { return s_localFrameDatas[Gfx::rg.currentFrameIdx]; }

void Init()
{
    s_text2DPipeline = Gfx::PipelineManager::GetPipeline( "text2D" );
    s_font           = AssetManager::Get<Font>( "arial" );

    for ( int i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        s_localFrameDatas[i].textVB.Free();
        s_localFrameDatas[i].numTextCharacters = 0;
    }
}

void Shutdown()
{
    s_text2DPipeline = nullptr;
    s_font           = nullptr;

    for ( int i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        s_localFrameDatas[i].textVB.Free();
    }
}

void Draw2D( const TextDrawInfo& textDrawInfo, const char* str )
{
    if ( !str )
        return;

    u32 sLen = (u32)strlen( str );
    if ( !sLen )
        return;

    vec2 scale      = vec2( textDrawInfo.fontSize );
    float drawWidth = 0;
    for ( u32 i = 0; i < sLen; ++i )
    {
        char c                   = str[i];
        const Font::Glyph& glyph = s_font->GetGlyph( c );
        drawWidth += scale.x * glyph.advance;
    }
    // const Font::Glyph& finalGlyph = s_font->GetGlyph( str[sLen - 1] );
    // drawArea.x = drawArea - (finalGlyph.advance - finalGlyph.xMax);

    vec2 posOffset = vec2( 0 );
    if ( textDrawInfo.justification == Justification::CENTER )
        posOffset.x -= drawWidth / 2.0f;
    else if ( textDrawInfo.justification == Justification::RIGHT )
        posOffset.x -= drawWidth;

    vec2 pos = textDrawInfo.pos + posOffset;
    for ( u32 i = 0; i < sLen; ++i )
    {
        char c                   = str[i];
        const Font::Glyph& glyph = s_font->GetGlyph( c );

        vec2 p    = pos + scale * vec2( glyph.bearing.x, glyph.sizeInAtlas.y - glyph.bearing.y );
        vec2 size = scale * glyph.sizeInAtlas;

        vec4 vertices[6] = {
            {p.x,          p.y,          0.0f, 0.0f},
            {p.x,          p.y + size.y, 0.0f, 1.0f},
            {p.x + size.x, p.y + size.y, 1.0f, 1.0f},

            {p.x,          p.y,          0.0f, 0.0f},
            {p.x + size.x, p.y + size.y, 1.0f, 1.0f},
            {p.x + size.x, p.y,          1.0f, 0.0f}
        };

        pos.x += scale.x * glyph.advance;
    }
}

} // namespace PG::UI::Text
