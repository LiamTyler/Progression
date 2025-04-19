#include "ui/ui_text.hpp"
#include "asset/asset_manager.hpp"
#include "c_shared/text.h"
#include "core/dvars.hpp"
#include "renderer/debug_marker.hpp"
#include "renderer/r_globals.hpp"
#include "renderer/r_pipeline_manager.hpp"

namespace PG::UI::Text
{

// NOTE! keep these in sync with c_shared/dvar_defines.h
Dvar text_kerning( "text_kerning", true, "Enable/disable text kerning" );

using namespace Gfx;

static Pipeline* s_text2DPipeline;
static const Font* s_font;

struct PackedTextDrawData
{
    u32 numChars;
    u32 vertOffset;
    u32 unorm8Color;
    float unitRange; // = fontSize/fontSizeInAtlas*sdfPixelRange
    vec2 unitRange3D;
};

struct LocalFrameData
{
    Gfx::Buffer textVB;
    u32 numTextCharacters;

    std::vector<PackedTextDrawData> text2DDrawCalls;
};

#define MAX_TEXT_CHARS_PER_FRAME 65536

static LocalFrameData s_localFrameDatas[NUM_FRAME_OVERLAP];
static Pipeline* s_debugTextPipeline;
static GfxImage* s_debugTextFontAtlas;

inline LocalFrameData& LocalData() { return s_localFrameDatas[Gfx::rg.currentFrameIdx]; }

void Init()
{
    s_text2DPipeline = Gfx::PipelineManager::GetPipeline( "text2D" );
    PG_ASSERT( s_text2DPipeline );
    s_font = AssetManager::Get<Font>( "arial" );
    PG_ASSERT( s_font );

    for ( int i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        BufferCreateInfo textCI                = {};
        textCI.size                            = MAX_TEXT_CHARS_PER_FRAME * 4 * sizeof( vec2 );
        textCI.bufferUsage                     = BufferUsage::TRANSFER_DST | BufferUsage::STORAGE | BufferUsage::DEVICE_ADDRESS;
        textCI.flags                           = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        s_localFrameDatas[i].textVB            = rg.device.NewBuffer( textCI, "ui_text_VB_" + std::to_string( i ) );
        s_localFrameDatas[i].numTextCharacters = 0;
        s_localFrameDatas[i].text2DDrawCalls.reserve( 128 );
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

vec2 MeasureText( const char* text, u32 length, Font* font, float fontSize )
{
    if ( !text || !length )
        return vec2( 0 );

    const bool useKerning = text_kerning.GetBool();

    const float spaceWidth = font->GetGlyph( ' ' ).advance;
    float scale            = fontSize / ( font->metrics.ascenderY - font->metrics.descenderY );

    float maxWidth     = 0;
    float currentWidth = 0;
    for ( u32 i = 0; i < length; ++i )
    {
        char c = text[i];
        if ( c == '\r' )
            continue;

        if ( c == ' ' )
        {
            currentWidth += spaceWidth;
            continue;
        }
        else if ( c == '\t' )
        {
            currentWidth += 4 * spaceWidth;
            continue;
        }
        else if ( c == '\n' )
        {
            maxWidth     = Max( maxWidth, currentWidth );
            currentWidth = 0;
            continue;
        }

        const Font::Glyph& glyph = font->GetGlyph( c );
        if ( useKerning && i > 0 )
        {
            float kerning = s_font->GetKerning( text[i - 1], c );
            currentWidth += kerning;
        }

        currentWidth += glyph.advance;
    }
    maxWidth = Max( maxWidth, currentWidth );

    return scale * vec2( maxWidth, 1.0f );
}

void Draw2D( const TextDrawInfo& textDrawInfo, const char* str )
{
    if ( !str )
        return;

    u32 sLen = (u32)strlen( str );
    if ( !sLen )
        return;

    bool useKerning = text_kerning.GetBool();

    const float spaceWidth = s_font->GetGlyph( ' ' ).advance;
    vec2 displaySize       = vec2( rg.displayWidth, rg.displayHeight );
    float scale            = textDrawInfo.fontSize / ( s_font->metrics.ascenderY - s_font->metrics.descenderY );

    vec2 startPos = displaySize * textDrawInfo.pos;
    vec2 pos      = startPos;
    pos.y += scale;

    LocalFrameData& localData = LocalData();
    PackedTextDrawData drawData;
    drawData.vertOffset = localData.numTextCharacters;

    vec2* gpuData = localData.textVB.GetMappedPtr<vec2>();
    for ( u32 i = 0; i < sLen; ++i )
    {
        char c = str[i];
        if ( c == '\r' )
            continue;

        if ( c == ' ' )
        {
            pos.x += scale * spaceWidth;
            continue;
        }
        else if ( c == '\t' )
        {
            pos.x += scale * 4 * spaceWidth;
            continue;
        }
        else if ( c == '\n' )
        {
            pos.x = startPos.x;
            pos.y += scale * s_font->metrics.lineHeight;
            continue;
        }

        const Font::Glyph& glyph = s_font->GetGlyph( c );

        vec2 quadMin = scale * glyph.planeMin;
        vec2 quadMax = scale * glyph.planeMax;

        gpuData[4 * localData.numTextCharacters + 0] = pos + quadMin;
        gpuData[4 * localData.numTextCharacters + 1] = quadMax - quadMin;
        gpuData[4 * localData.numTextCharacters + 2] = glyph.uvMin;
        gpuData[4 * localData.numTextCharacters + 3] = glyph.uvMax - glyph.uvMin;

        localData.numTextCharacters += 1;

        pos.x += scale * glyph.advance;
        if ( useKerning && i < ( sLen - 1 ) )
        {
            float kerning = s_font->GetKerning( c, str[i + 1] );
            pos.x += scale * kerning;
        }
    }

    drawData.numChars = localData.numTextCharacters - drawData.vertOffset;
    if ( drawData.numChars == 0 )
        return;

    vec2 atlasSize       = vec2( s_font->fontAtlasTexture.width, s_font->fontAtlasTexture.height );
    drawData.unitRange   = textDrawInfo.fontSize / s_font->metrics.fontSize * s_font->metrics.maxSignedDistanceRange;
    drawData.unitRange3D = vec2( s_font->metrics.maxSignedDistanceRange ) / atlasSize;
    drawData.unorm8Color = UNormFloat4ToU32( textDrawInfo.color );
    localData.text2DDrawCalls.push_back( drawData );
}

void Render( Gfx::CommandBuffer& cmdBuf )
{
    LocalFrameData& localData = LocalData();
    if ( localData.numTextCharacters == 0 || localData.text2DDrawCalls.empty() )
    {
        localData.numTextCharacters = 0;
        return;
    }

    cmdBuf.BindPipeline( s_text2DPipeline );
    cmdBuf.BindGlobalDescriptors();
    cmdBuf.SetViewport( SceneSizedViewport( false ) );
    cmdBuf.SetScissor( SceneSizedScissor() );

    GpuData::TextDrawData constants;
    constants.projMatrix      = glm::ortho<float>( 0, (float)rg.displayWidth, 0, (float)rg.displayHeight );
    constants.sdfFontAtlasTex = s_font->fontAtlasTexture.gpuTexture.GetBindlessIndex();
    u64 vertexBuffer          = localData.textVB.GetDeviceAddress();

    u64 vbOffset = 0;
    for ( const PackedTextDrawData& drawCall : localData.text2DDrawCalls )
    {
        constants.vertexBuffer = vertexBuffer + vbOffset;
        constants.unitRange    = drawCall.unitRange;
        constants.unitRange3D  = drawCall.unitRange3D;
        constants.packedColor  = drawCall.unorm8Color;
        cmdBuf.PushConstants( constants );
        cmdBuf.Draw( 0, 6 * drawCall.numChars );

        vbOffset += 4 * sizeof( vec2 ) * drawCall.numChars;
    }

    localData.numTextCharacters = 0;
    localData.text2DDrawCalls.clear();
}

} // namespace PG::UI::Text
