#include "debug_draw.hpp"
#include "debug_marker.hpp"
#include "r_globals.hpp"
#include "r_pipeline_manager.hpp"

#define MAX_DEBUG_LINES 256

namespace PG::Gfx::DebugDraw
{

static float GetPackedColor( Color color )
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

    u32 p = packed[Underlying( color )];
    return *reinterpret_cast<float*>( &p );
}

struct LocalFrameData
{
    Buffer debugLinesGPU;
    std::vector<float> debugLinesCPU; // Line is 7 floats: vec3 p0, vec3 p1, uint packedColor
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
        s_localFrameDatas[i].debugLinesCPU.reserve( MAX_DEBUG_LINES * 7 );

        BufferCreateInfo dbgLinesBufInfo   = {};
        dbgLinesBufInfo.size               = MAX_DEBUG_LINES * 7 * sizeof( float );
        dbgLinesBufInfo.bufferUsage        = BufferUsage::STORAGE | BufferUsage::TRANSFER_DST | BufferUsage::DEVICE_ADDRESS;
        dbgLinesBufInfo.flags              = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        s_localFrameDatas[i].debugLinesGPU = rg.device.NewBuffer( dbgLinesBufInfo, "debugLines" );
    }
}

void Shutdown()
{
    for ( int i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        s_localFrameDatas[i].debugLinesGPU.Free();
    }
}

void StartFrame()
{
    LocalData().warnedThisFrame = false;
    LocalData().debugLinesCPU.clear();
}

void Draw( CommandBuffer& cmdBuf )
{
    LocalFrameData& localData = LocalData();
    float* linesGPU           = localData.debugLinesGPU.GetMappedPtr<float>();
    memcpy( linesGPU, localData.debugLinesCPU.data(), localData.debugLinesCPU.size() * 7 * sizeof( float ) );

    VkDeviceAddress vertexBuffer = localData.debugLinesGPU.GetDeviceAddress();

    cmdBuf.BindPipeline( PipelineManager::GetPipeline( "debug_lines" ) );
    cmdBuf.BindGlobalDescriptors();

    cmdBuf.SetViewport( SceneSizedViewport() );
    cmdBuf.SetScissor( SceneSizedScissor() );
    cmdBuf.PushConstants( vertexBuffer );

    PG_DEBUG_MARKER_INSERT_CMDBUF( cmdBuf, "Draw Debug Lines" );
    u32 numVerts = 2 * (u32)localData.debugLinesCPU.size() / 7;
    cmdBuf.Draw( 0, numVerts );
}

void AddLine( vec3 begin, vec3 end, Color color )
{
    u64 numLines = LocalData().debugLinesCPU.size();
    if ( numLines >= MAX_DEBUG_LINES )
    {
        if ( !LocalData().warnedThisFrame )
        {
            LOG_WARN( "Reached the max number of debug lines on this frame. Skipping all future lines" );
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
}

#else  // #if USING( DEBUG_DRAW )

void Init() {}
void Shutdown() {}
void StartFrame() {}

void AddLine( vec3 begin, vec3 end )
{
    PG_UNUSED( begin );
    PG_UNUSED( end );
}
#endif // #else // #if USING( DEBUG_DRAW )

} // namespace PG::Gfx::DebugDraw
