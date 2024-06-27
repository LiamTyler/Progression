#include "debug_draw.hpp"
#include "debug_marker.hpp"
#include "r_globals.hpp"
#include "r_pipeline_manager.hpp"
#include <bit>

#define MAX_DEBUG_PRIMS 512

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
    bool warnedThisFrame;
};

static LocalFrameData s_localFrameDatas[NUM_FRAME_OVERLAP];

#if USING( DEBUG_DRAW )
inline LocalFrameData& LocalData() { return s_localFrameDatas[rg.currentFrameIdx]; }

void Init()
{
    for ( int i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        s_localFrameDatas[i].warnedThisFrame = false;
        s_localFrameDatas[i].totalPrims      = 0;
        s_localFrameDatas[i].debugLinesCPU.reserve( MAX_DEBUG_PRIMS * 7 );
        s_localFrameDatas[i].debugAABBsCPU.reserve( MAX_DEBUG_PRIMS * 7 );

        BufferCreateInfo dbgLinesBufInfo   = {};
        dbgLinesBufInfo.size               = MAX_DEBUG_PRIMS * 7 * sizeof( f32 );
        dbgLinesBufInfo.bufferUsage        = BufferUsage::STORAGE | BufferUsage::TRANSFER_DST | BufferUsage::DEVICE_ADDRESS;
        dbgLinesBufInfo.flags              = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        s_localFrameDatas[i].debugLinesGPU = rg.device.NewBuffer( dbgLinesBufInfo, "debugLines" );
        s_localFrameDatas[i].debugAABBsGPU = rg.device.NewBuffer( dbgLinesBufInfo, "debugAABBs" );
    }
}

void Shutdown()
{
    for ( int i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        s_localFrameDatas[i].debugLinesGPU.Free();
        s_localFrameDatas[i].debugAABBsGPU.Free();
    }
}

void StartFrame()
{
    LocalData().totalPrims = 0;
    LocalData().debugLinesCPU.clear();
    LocalData().debugAABBsCPU.clear();
}

void Draw( CommandBuffer& cmdBuf )
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

void AddLine( vec3 begin, vec3 end, Color color )
{
    if ( LocalData().totalPrims >= MAX_DEBUG_PRIMS )
    {
        if ( !LocalData().warnedThisFrame )
        {
            LOG_WARN( "Reached the max number of debug primitives on this frame. Skipping all future debug prims" );
            LocalData().warnedThisFrame = true;
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
        if ( !LocalData().warnedThisFrame )
        {
            LOG_WARN( "Reached the max number of debug primitives on this frame. Skipping all future debug prims" );
            LocalData().warnedThisFrame = true;
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
        if ( !LocalData().warnedThisFrame )
        {
            LOG_WARN( "Reached the max number of debug primitives on this frame. Skipping all future debug prims" );
            LocalData().warnedThisFrame = true;
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
#endif // #else // #if USING( DEBUG_DRAW )

} // namespace PG::Gfx::DebugDraw
