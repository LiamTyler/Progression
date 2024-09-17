#include "debug_draw.hpp"
#include "asset/asset_manager.hpp"
#include "c_shared/debug_text.h"
#include "debug_marker.hpp"
#include "r_globals.hpp"
#include "r_pipeline_manager.hpp"
#include "shared/float_conversions.hpp"
#include <bit>

#define MAX_DEBUG_PRIMS 512
#define MAX_DEBUG_TEXT_CHARACTERS 65536u

namespace PG::Gfx::DebugDraw
{

static f32 GetPackedColor( Color color )
{
    static u32 packed[] = {
        0xFF000000, // BLACK,
        0xFF7F7F7F, // GRAY,
        0xFFFFFFFF, // WHITE,
        0xFF0000FF, // RED,
        0xFF00FF00, // GREEN,
        0xFFFF0000, // BLUE,
        0xFF00FFFF, // YELLOW,
        0xFFFF00FF, // MAGENTA,
        0xFFFFFF00, // CYAN
    };
    static_assert( ARRAY_COUNT( packed ) == Underlying( Color::COUNT ) );

    return std::bit_cast<f32>( packed[Underlying( color )] );
}

struct LocalFrameData
{
    Buffer debugLinesGPU;
    Buffer debugAABBsGPU;
    // Both an AABB and a Line are 7 floats: vec3 p0, vec3 p1, uint packedColor
    std::vector<f32> debugLinesCPU;
    std::vector<f32> debugAABBsCPU;
    u32 totalPrims;
    bool warnedMaxDebugLinesThisFrame;

    Buffer debugTextVB;
    u32 numDebugTextCharacters;
    bool warnedMaxDebugTextThisFrame;
};

static LocalFrameData s_localFrameDatas[NUM_FRAME_OVERLAP];
static Pipeline* s_debugTextPipeline;
static GfxImage* s_debugTextFontAtlas;

#if USING( DEBUG_DRAW )
inline LocalFrameData& LocalData() { return s_localFrameDatas[rg.currentFrameIdx]; }

void Init()
{
    for ( int i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        std::string iStr                                  = std::to_string( i );
        s_localFrameDatas[i].warnedMaxDebugLinesThisFrame = false;
        s_localFrameDatas[i].totalPrims                   = 0;
        s_localFrameDatas[i].debugLinesCPU.reserve( MAX_DEBUG_PRIMS * 7 );
        s_localFrameDatas[i].debugAABBsCPU.reserve( MAX_DEBUG_PRIMS * 7 );

        BufferCreateInfo dbgLinesBufInfo   = {};
        dbgLinesBufInfo.size               = MAX_DEBUG_PRIMS * 7 * sizeof( f32 );
        dbgLinesBufInfo.bufferUsage        = BufferUsage::STORAGE | BufferUsage::TRANSFER_DST | BufferUsage::DEVICE_ADDRESS;
        dbgLinesBufInfo.flags              = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        s_localFrameDatas[i].debugLinesGPU = rg.device.NewBuffer( dbgLinesBufInfo, "debugLines_" + iStr );
        s_localFrameDatas[i].debugAABBsGPU = rg.device.NewBuffer( dbgLinesBufInfo, "debugAABBs_" + iStr );

        s_localFrameDatas[i].numDebugTextCharacters = 0;
        BufferCreateInfo dbgTextCI                  = {};
        dbgTextCI.size                              = MAX_DEBUG_TEXT_CHARACTERS * sizeof( vec4 );
        dbgTextCI.bufferUsage                       = BufferUsage::TRANSFER_DST | BufferUsage::STORAGE | BufferUsage::DEVICE_ADDRESS;
        dbgTextCI.flags                             = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        s_localFrameDatas[i].debugTextVB            = rg.device.NewBuffer( dbgTextCI, "debug_text_VB_" + iStr );
    }

    s_debugTextPipeline  = PipelineManager::GetPipeline( "debug_text" );
    s_debugTextFontAtlas = AssetManager::Get<GfxImage>( "debug_font" );
}

void Shutdown()
{
    for ( int i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        s_localFrameDatas[i].debugLinesGPU.Free();
        s_localFrameDatas[i].debugAABBsGPU.Free();
        s_localFrameDatas[i].debugTextVB.Free();
    }
}

void StartFrame()
{
    LocalData().totalPrims = 0;
    LocalData().debugLinesCPU.clear();
    LocalData().debugAABBsCPU.clear();
    LocalData().warnedMaxDebugLinesThisFrame = false;
    LocalData().numDebugTextCharacters       = 0;
    LocalData().warnedMaxDebugTextThisFrame  = false;
}

void DrawText( CommandBuffer& cmdBuf )
{
    LocalFrameData& localData = LocalData();
    if ( localData.numDebugTextCharacters == 0 )
        return;

    cmdBuf.BindPipeline( s_debugTextPipeline );
    cmdBuf.BindGlobalDescriptors();
    cmdBuf.SetViewport( SceneSizedViewport( false ) );
    cmdBuf.SetScissor( SceneSizedScissor() );

    GpuData::TextDrawData constants;
    constants.vertexBuffer = localData.debugTextVB.GetDeviceAddress();
    constants.fontTexture  = s_debugTextFontAtlas->gpuTexture.GetBindlessIndex();
    cmdBuf.PushConstants( constants );

    PG_DEBUG_MARKER_INSERT_CMDBUF( cmdBuf, "Draw Debug Text" );
    cmdBuf.Draw( 0, 6 * localData.numDebugTextCharacters );
}

void DrawLines( CommandBuffer& cmdBuf )
{
    LocalFrameData& localData = LocalData();
    if ( localData.totalPrims == 0 )
        return;

    cmdBuf.BindPipeline( PipelineManager::GetPipeline( "debug_lines" ) );
    cmdBuf.BindGlobalDescriptors();

    cmdBuf.SetViewport( SceneSizedViewport() );
    cmdBuf.SetScissor( SceneSizedScissor() );

    struct PushConstants
    {
        VkDeviceAddress vertexBuffer;
        u32 mode;
        u32 _pad;
    };
    PushConstants constants;

    if ( !localData.debugLinesCPU.empty() )
    {
        PG_DEBUG_MARKER_INSERT_CMDBUF( cmdBuf, "Draw Debug Lines" );
        f32* linesGPU = localData.debugLinesGPU.GetMappedPtr<float>();
        memcpy( linesGPU, localData.debugLinesCPU.data(), localData.debugLinesCPU.size() * 7 * sizeof( f32 ) );

        constants.vertexBuffer = localData.debugLinesGPU.GetDeviceAddress();
        constants.mode         = 0;
        cmdBuf.PushConstants( constants );
        u32 numLines = (u32)localData.debugLinesCPU.size() / 7;
        cmdBuf.Draw( 0, 2 * numLines );
    }

    if ( !localData.debugAABBsCPU.empty() )
    {
        PG_DEBUG_MARKER_INSERT_CMDBUF( cmdBuf, "Draw Debug AABBs" );
        f32* aabbsGPU = localData.debugAABBsGPU.GetMappedPtr<float>();
        memcpy( aabbsGPU, localData.debugAABBsCPU.data(), localData.debugAABBsCPU.size() * 7 * sizeof( f32 ) );

        constants.vertexBuffer = localData.debugAABBsGPU.GetDeviceAddress();
        constants.mode         = 1;
        cmdBuf.PushConstants( constants );
        u32 numAABBs = (u32)localData.debugAABBsCPU.size() / 7;
        cmdBuf.Draw( 0, 24, numAABBs );
    }
}

void Draw3D( CommandBuffer& cmdBuf ) { DrawLines( cmdBuf ); }

void Draw2D( CommandBuffer& cmdBuf ) { DrawText( cmdBuf ); }

void AddLine( vec3 begin, vec3 end, Color color )
{
    if ( LocalData().totalPrims >= MAX_DEBUG_PRIMS )
    {
        if ( !LocalData().warnedMaxDebugLinesThisFrame )
        {
            LOG_WARN( "Reached the max number of debug primitives on this frame. Skipping all future debug prims" );
            LocalData().warnedMaxDebugLinesThisFrame = true;
        }
        return;
    }

    LocalData().debugLinesCPU.push_back( begin.x );
    LocalData().debugLinesCPU.push_back( begin.y );
    LocalData().debugLinesCPU.push_back( begin.z );
    LocalData().debugLinesCPU.push_back( end.x );
    LocalData().debugLinesCPU.push_back( end.y );
    LocalData().debugLinesCPU.push_back( end.z );
    LocalData().debugLinesCPU.push_back( GetPackedColor( color ) );
    LocalData().totalPrims++;
}

void AddAABB( const AABB& aabb, Color color )
{
    if ( LocalData().totalPrims >= MAX_DEBUG_PRIMS )
    {
        if ( !LocalData().warnedMaxDebugLinesThisFrame )
        {
            LOG_WARN( "Reached the max number of debug primitives on this frame. Skipping all future debug prims" );
            LocalData().warnedMaxDebugLinesThisFrame = true;
        }
        return;
    }

    LocalData().debugAABBsCPU.push_back( aabb.min.x );
    LocalData().debugAABBsCPU.push_back( aabb.min.y );
    LocalData().debugAABBsCPU.push_back( aabb.min.z );
    LocalData().debugAABBsCPU.push_back( aabb.max.x );
    LocalData().debugAABBsCPU.push_back( aabb.max.y );
    LocalData().debugAABBsCPU.push_back( aabb.max.z );
    LocalData().debugAABBsCPU.push_back( GetPackedColor( color ) );
    LocalData().totalPrims++;
}

void AddFrustum( const Frustum& frustum, Color color )
{
    if ( LocalData().totalPrims >= MAX_DEBUG_PRIMS )
    {
        if ( !LocalData().warnedMaxDebugLinesThisFrame )
        {
            LOG_WARN( "Reached the max number of debug primitives on this frame. Skipping all future debug prims" );
            LocalData().warnedMaxDebugLinesThisFrame = true;
        }
        return;
    }

    AddLine( frustum.corners[0], frustum.corners[1], color );
    AddLine( frustum.corners[1], frustum.corners[2], color );
    AddLine( frustum.corners[2], frustum.corners[3], color );
    AddLine( frustum.corners[3], frustum.corners[0], color );

    // AddLine( frustum.corners[4], frustum.corners[5], color );
    // AddLine( frustum.corners[5], frustum.corners[6], color );
    // AddLine( frustum.corners[6], frustum.corners[7], color );
    // AddLine( frustum.corners[7], frustum.corners[4], color );

    AddLine( frustum.corners[0], frustum.corners[4], color );
    AddLine( frustum.corners[1], frustum.corners[5], color );
    AddLine( frustum.corners[2], frustum.corners[6], color );
    AddLine( frustum.corners[3], frustum.corners[7], color );
}

static constexpr f32 FONT_WIDTH         = 32;
static constexpr f32 FONT_HEIGHT        = 32;
static constexpr f32 FONT_ADVANCE_WIDTH = 10;
static constexpr u32 ATLAS_ROWS         = 8;
static constexpr u32 ATLAS_COLS         = 16;
static const vec2 INV_ATLAS_SIZE( 1.0f / ATLAS_COLS, 1.0f / ATLAS_ROWS );

void AddText2D( vec2 normalizedPos, Color color, const char* str )
{
    LocalFrameData& localData = LocalData();
    u32 stringLen             = (u32)strlen( str );
    if ( !stringLen )
        return;

    u32 startIndex = localData.numDebugTextCharacters;
    u32 charsToAdd = Min( MAX_DEBUG_TEXT_CHARACTERS, localData.numDebugTextCharacters + stringLen );
    localData.numDebugTextCharacters += charsToAdd;
    if ( charsToAdd != stringLen && !LocalData().warnedMaxDebugTextThisFrame )
    {
        LOG_WARN( "Reached the max number of debug text characters on this frame. Skipping all future debug text" );
        LocalData().warnedMaxDebugTextThisFrame = true;
    }
    if ( !charsToAdd )
        return;

    f32 packedColor             = GetPackedColor( color );
    static vec2 quadPositions[] = {
        vec2( 0, 0 ),
        vec2( 0, 1 ),
        vec2( 1, 1 ),
        vec2( 0, 0 ),
        vec2( 1, 1 ),
        vec2( 1, 0 ),
    };

    vec2 scaleFactor( FONT_WIDTH / rg.displayWidth, FONT_HEIGHT / rg.displayHeight );
    vec4* gpuData = localData.debugTextVB.GetMappedPtr<vec4>();
    for ( u32 charIdx = 0; charIdx < charsToAdd; ++charIdx )
    {
        u32 c           = str[charIdx] - 32;
        u32 col         = c % ATLAS_COLS;
        u32 row         = c / ATLAS_COLS;
        vec2 topLeft    = vec2( col, row ) * INV_ATLAS_SIZE;
        u32 globalIndex = 6 * ( startIndex + charIdx );
        for ( u32 i = 0; i < 6; ++i )
        {
            vec2 uv = topLeft + quadPositions[i] * INV_ATLAS_SIZE;
            vec2 p  = normalizedPos + quadPositions[i] * scaleFactor;
            p.x += charIdx * FONT_ADVANCE_WIDTH / rg.displayWidth;
            f32 packedUV             = Float32ToFloat16AsFloat( uv.x, uv.y );
            gpuData[globalIndex + i] = vec4( p, packedUV, packedColor );
        }
    }
}

#else  // #if USING( DEBUG_DRAW )

void Init() {}
void Shutdown() {}
void StartFrame() {}
void Draw( CommandBuffer& cmdBuf ) { PG_UNUSED( cmdBuf ); }
void AddLine( vec3 begin, vec3 end, Color color )
{
    PG_UNUSED( begin );
    PG_UNUSED( end );
    PG_UNUSED( color );
}

void AddAABB( const AABB& aabb, Color color )
{
    PG_UNUSED( aabb );
    PG_UNUSED( color );
}

void AddText( vec2 normalizedPos, Color color, const char* str )
{
    PG_UNUSED( normalizedPos );
    PG_UNUSED( color );
    PG_UNUSED( str );
}
#endif // #else // #if USING( DEBUG_DRAW )

} // namespace PG::Gfx::DebugDraw
