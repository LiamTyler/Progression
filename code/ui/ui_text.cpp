#include "ui/ui_text.hpp"
#include "asset/asset_manager.hpp"
#include "c_shared/text.h"
#include "renderer/debug_marker.hpp"
#include "renderer/r_globals.hpp"
#include "renderer/r_pipeline_manager.hpp"

namespace PG::UI::Text
{

using namespace Gfx;

static Pipeline* s_text2DPipeline;
static const Font* s_font;

struct LocalFrameData
{
    Gfx::Buffer textVB;
    u32 numTextCharacters;
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

    vec2 displaySize = vec2( rg.displayWidth, rg.displayHeight );
    vec2 scale       = vec2( textDrawInfo.fontSize );
    float drawWidth  = 0;
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

    vec2* gpuData = LocalData().textVB.GetMappedPtr<vec2>();
    vec2 pos      = displaySize * textDrawInfo.pos + posOffset;
    for ( u32 i = 0; i < sLen; ++i )
    {
        char c                   = str[i];
        const Font::Glyph& glyph = s_font->GetGlyph( c );

        vec2 p = pos + scale * vec2( glyph.bearing.x, glyph.nonSDFSize.y - glyph.bearing.y );

        gpuData[4 * LocalData().numTextCharacters + 0] = p;
        gpuData[4 * LocalData().numTextCharacters + 1] = scale * glyph.nonSDFSize;
        gpuData[4 * LocalData().numTextCharacters + 2] = glyph.positionInAtlas;
        gpuData[4 * LocalData().numTextCharacters + 3] = glyph.sizeInAtlas;

        LocalData().numTextCharacters += 1;

        // vec2 uvUL = glyph.positionInAtlas;
        // vec2 uvBR = glyph.positionInAtlas + glyph.sizeInAtlas;
        // vec2 size = scale * glyph.sizeInAtlas;
        // vec4 vertices[6] = {
        //     {p.x,           p.y,          uvUL.x, uvUL.y},
        //     { p.x,          p.y + size.y, uvUL.x, uvBR.y},
        //     { p.x + size.x, p.y + size.y, uvBR.x, uvBR.y},
        //
        //     { p.x,          p.y,          uvUL.x, uvUL.y},
        //     { p.x + size.x, p.y + size.y, uvBR.x, uvBR.y},
        //     { p.x + size.x, p.y,          uvBR.x, uvUL.y}
        // };

        pos.x += scale.x * glyph.advance;
    }
}

void Render( Gfx::CommandBuffer& cmdBuf )
{
    LocalFrameData& localData = LocalData();
    if ( localData.numTextCharacters == 0 )
        return;

    cmdBuf.BindPipeline( s_text2DPipeline );
    cmdBuf.BindGlobalDescriptors();
    cmdBuf.SetViewport( SceneSizedViewport( false ) );
    cmdBuf.SetScissor( SceneSizedScissor() );

    GpuData::TextDrawData constants;
    constants.vertexBuffer    = localData.textVB.GetDeviceAddress();
    constants.sdfFontAtlasTex = s_font->fontAtlasTexture.gpuTexture.GetBindlessIndex();
    constants.invScale        = vec2( 1.0f / rg.displayWidth, 1.0f / rg.displayHeight );
    cmdBuf.PushConstants( constants );

    PG_DEBUG_MARKER_INSERT_CMDBUF( cmdBuf, "Draw Text2D" );
    cmdBuf.Draw( 0, 6 * localData.numTextCharacters );
    localData.numTextCharacters = 0;
}

} // namespace PG::UI::Text
